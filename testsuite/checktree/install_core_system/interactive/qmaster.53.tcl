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

# install qmaster check 
#                                                             max. column:     |
#****** install_core_system/install_qmaster() ******
# 
#  NAME
#     install_qmaster -- ??? 
#
#  SYNOPSIS
#     install_qmaster { } 
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
proc install_qmaster {} {
   global ts_config
 global CHECK_USER check_errstr 
 global CHECK_CORE_MASTER CORE_INSTALLED CORE_INSTALLED CHECK_OUTPUT 
 global CHECK_TESTSUITE_ROOT env CHECK_COMMD_PORT local_master_spool_set
 global check_use_installed_system CHECK_ADMIN_USER_SYSTEM CHECK_DEFAULT_DOMAIN
 global CHECK_DEBUG_LEVEL CHECK_QMASTER_INSTALL_OPTIONS CHECK_COMMD_PORT
 global CHECK_REPORT_EMAIL_TO CHECK_MAIN_RESULTS_DIR CHECK_FIRST_FOREIGN_SYSTEM_USER
 global CHECK_SECOND_FOREIGN_SYSTEM_USER CHECK_REPORT_EMAIL_TO
 global CHECK_PROTOCOL_DIR

 puts $CHECK_OUTPUT "install qmaster ($ts_config(product_type) system) on host $CHECK_CORE_MASTER ..."

 if { $check_use_installed_system != 0 } {
    set_error "0" "install_qmaster - no need to install qmaster on host $CHECK_CORE_MASTER - noinst parameter is set"
    puts "no need to install qmaster on host $CHECK_CORE_MASTER, noinst parameter is set"
    set CORE_INSTALLED "" 
    if {[startup_qmaster] == 0} {
      lappend CORE_INSTALLED $CHECK_CORE_MASTER
      write_install_list
    }
    return
 }

 set CORE_INSTALLED ""
 write_install_list

 set_error "0" "install_qmaster - no errors"
 if {[file isfile "$ts_config(product_root)/install_qmaster"] != 1} {
    set_error "-1" "install_qmaster - install_qmaster file not found"
    return
 }

 #dump hostlist to file
 set admin_hosts "$ts_config(all_nodes) $ts_config(shadowd_hosts)"
 set admin_hosts [lsort -unique $admin_hosts]

 set host_file_name "$CHECK_PROTOCOL_DIR/hostlist"
 set f [open $host_file_name w]
 foreach host $admin_hosts {
    puts $f $host
 }
 close $f


 set HIT_RETURN_TO_CONTINUE       [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_HIT_RETURN_TO_CONTINUE] ]
 set NOT_COMPILED_IN_SECURE_MODE  [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_NOT_COMPILED_IN_SECURE_MODE] ] 
 set ENTER_HOSTS                  [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ENTER_HOSTS] ]
 set MASTER_INSTALLATION_COMPLETE [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_MASTER_INSTALLATION_COMPLETE] ]
 set ENTER_A_RANGE                [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ENTER_A_RANGE] ]
 set PREVIOUS_SCREEN              [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_PREVIOUS_SCREEN] ]
 set FILE_FOR_HOSTLIST            [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_FILE_FOR_HOSTLIST] ]
 set FINISHED_ADDING_HOSTS        [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_FINISHED_ADDING_HOSTS] ]
 set FILENAME_FOR_HOSTLIST        [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_FILENAME_FOR_HOSTLIST] ]
 set CREATE_NEW_CONFIGURATION     [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_CREATE_NEW_CONFIGURATION] ]
 set INSTALL_SCRIPT               [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_INSTALL_SCRIPT] ]
 set ANSWER_YES                   [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ANSWER_YES] ]
 set ANSWER_NO                    [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ANSWER_NO] ]
 set ENTER_DEFAULT_DOMAIN         [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ENTER_DEFAULT_DOMAIN] ]
 set CONFIGURE_DEFAULT_DOMAIN     [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_CONFIGURE_DEFAULT_DOMAIN] ] 
 set PKGADD_QUESTION              [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_PKGADD_QUESTION] ]
 set MESSAGES_LOGGING             [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_MESSAGES_LOGGING] ]
 set OTHER_SPOOL_DIR              [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_OTHER_SPOOL_DIR] ]
 set OTHER_USER_ID_THAN_ROOT      [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_OTHER_USER_ID_THAN_ROOT] ]
 set INSTALL_AS_ADMIN_USER        [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_INSTALL_AS_ADMIN_USER] "$CHECK_USER" ]
 set ADMIN_USER_ACCOUNT           [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ADMIN_USER_ACCOUNT] "$CHECK_USER\r\n" ]
 set USE_CONFIGURATION_PARAMS     [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_USE_CONFIGURATION_PARAMS] ]
 set INSTALL_GE_NOT_AS_ROOT       [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_INSTALL_GE_NOT_AS_ROOT] ]
 set IF_NOT_OK_STOP_INSTALLATION  [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_IF_NOT_OK_STOP_INSTALLATION] ]
 set DNS_DOMAIN_QUESTION          [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_DNS_DOMAIN_QUESTION] ] 
 set ENTER_SPOOL_DIR_OR_HIT_RET   [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_ENTER_SPOOL_DIR_OR_HIT_RET] "*"]
 set USING_GID_RANGE_HIT_RETURN   [translate $CHECK_CORE_MASTER 0 1 0 [sge_macro DISTINST_USING_GID_RANGE_HIT_RETURN] "*"]



 cd "$ts_config(product_root)"

 set prod_type_var "SGE_ROOT"

 set feature_install_options ""
 if { $ts_config(product_feature) == "csp" } {
    append feature_install_options "-csp"
 }

 if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
    set id [open_remote_spawn_process "$CHECK_CORE_MASTER" "root"  "cd $$prod_type_var;./install_qmaster" "$CHECK_QMASTER_INSTALL_OPTIONS $feature_install_options" ]
 } else {
    puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
    set id [open_remote_spawn_process "$CHECK_CORE_MASTER" "$CHECK_USER"  "cd $$prod_type_var;./install_qmaster" "$CHECK_QMASTER_INSTALL_OPTIONS $feature_install_options" ]
 }
 set sp_id [ lindex $id 1 ] 
 

 log_user 1
 puts $CHECK_OUTPUT "cd $$prod_type_var;./install_qmaster $CHECK_QMASTER_INSTALL_OPTIONS $feature_install_options"

 set hostcount 0

 set do_log_output 0 ;# _LOG
 if { $CHECK_DEBUG_LEVEL == 2 } {
   set do_log_output  1 ;# 1
 }

 while {1} {
 if {$do_log_output == 1} {
   flush stdout
   flush $CHECK_OUTPUT
   puts "-->testsuite: press RETURN"
   set anykey [wait_for_enter 1]
 }
    log_user 1
    set timeout 300
    expect {
       flush stdout
       flush $CHECK_OUTPUT 
       -i $sp_id full_buffer {
          set_error "-1" "install_qmaster - buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
          close_spawn_process $id; 
          return;
       }   

       -i $sp_id eof { 
          set_error "-1" "install_qmaster - unexpected eof"; 
          close_spawn_process $id;
          return;  
       }

       -i $sp_id "coredump" {
          set_error "-2" "install_qmaster - coredump";
          close_spawn_process $id
          return
       }

       -i $sp_id timeout { 
          set_error "-1" "install_qmaster - timeout while waiting for output"; 
          close_spawn_process $id;
          return;  
       }

       -i $sp_id "orry" { 
          set_error "-1" "install_qmaster - wrong root password"
          close_spawn_process $id;
          return;
       }

       -i $sp_id "issing" { 
          set_error "-1" "install_qmaster - missing binary error"
          close_spawn_process $id;
          return;
       }

       -i $sp_id "xit." {
          set_error "-1" "install_qmaster - installation failed"
          close_spawn_process $id; 
          return;
       }

       -i $sp_id $ADMIN_USER_ACCOUNT {
          set real_admin_user $expect_out(0,string)
          set real_help [ split $real_admin_user "=" ]
          set real_admin_user [ string trim [ lindex $real_help 1 ]]
          
          puts $CHECK_OUTPUT "\n -->testsuite: admin user is \"$real_admin_user\""
          if { [string compare $real_admin_user $CHECK_USER] != 0 } {
             set_error "-1" "install_qmaster - admin user \"$real_admin_user\" is different from CHECK_USER \"$CHECK_USER\"" 
             close_spawn_process $id;
             return;
          }
       }

       -i $sp_id "o you want to recreate your SGE CA infrastructure" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >y<(1)"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "y\n"
          continue;
       }

       -i $sp_id "enter your two letter country code" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >DE<"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "DE\n"
          continue;
       }

       -i $sp_id "lease enter your state" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >Bavaria<"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "Bavaria\n"
          continue;
       }
 
       -i $sp_id "lease enter your location" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >Regensburg<"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "Regensburg\n"
          continue;
       }

       -i $sp_id "lease enter the name of your organization" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >Sun Microsystems<"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "Sun Microsystems\n"
          continue;
       }

       -i $sp_id "lease enter your organizational unit" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >Testsystem at port $CHECK_COMMD_PORT<"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "Testsystem at port $CHECK_COMMD_PORT\n"
          continue;
       }

       -i $sp_id "lease enter the email address of the CA administrator" { 
          if { $CHECK_REPORT_EMAIL_TO == "none" } {
             set CA_admin_mail "$CHECK_USER@sun.com"
          } else {
             set CA_admin_mail $CHECK_REPORT_EMAIL_TO
          }
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$CA_admin_mail<"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$CA_admin_mail\n"
          continue;
       }

       -i $sp_id "o you want to use these data" { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >y<(2)"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "y\n"
          continue;
       }

       -i $sp_id $DNS_DOMAIN_QUESTION { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(4)"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_YES\n"
          continue;
       }

       -i $sp_id $INSTALL_AS_ADMIN_USER { 
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(5)"
          if {$do_log_output == 1} {
             puts "press RETURN"
             set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_YES\n"
          continue;
       }

       -i $sp_id $USE_CONFIGURATION_PARAMS {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(1)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }

       -i $sp_id "Verifying and setting file permissions and owner in" {
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          continue;
       }

       -i $sp_id "Please hit <RETURN> to continue once you set your file permissions" {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >return<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "\n"
          continue;
       }

       -i $sp_id $IF_NOT_OK_STOP_INSTALLATION {
          if { $CHECK_ADMIN_USER_SYSTEM != 0 } {
             puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
             if {$do_log_output == 1} {
                  puts "press RETURN"
                  set anykey [wait_for_enter 1]
             }
             send -i $sp_id "\n"
             continue;
          } else {
             set_error "-1" "install_qmaster - tried to install not as root"
             close_spawn_process $id; 
             return;
          }
       }

       -i $sp_id $INSTALL_GE_NOT_AS_ROOT {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(4)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }
       
       -i $sp_id $OTHER_USER_ID_THAN_ROOT {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(4)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }

       -i $sp_id $OTHER_SPOOL_DIR {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(5)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }

       -i $sp_id $MESSAGES_LOGGING {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }

          send -i $sp_id "\n"
          continue;
       }

       -i $sp_id $PKGADD_QUESTION {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(6)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }

          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }

       -i $sp_id $ENTER_SPOOL_DIR_OR_HIT_RET {
          puts $CHECK_OUTPUT "\n"
          set spooldir [get_local_spool_dir $CHECK_CORE_MASTER qmaster]
          if { $spooldir != "" } {
            # use local spool dir
            puts $CHECK_OUTPUT "\n -->testsuite: sending >$spooldir<"
            if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
            }
            send -i $sp_id "$spooldir\n"
            set local_master_spool_set 1
          } else {
            # use default spool dir
            puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
            if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
            }
            send -i $sp_id "\n"
          }
          continue;
       }

       -i $sp_id $CONFIGURE_DEFAULT_DOMAIN {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(7)"
          if {$do_log_output == 1} {
               puts "(2)press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }

       -i $sp_id $ENTER_DEFAULT_DOMAIN {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$CHECK_DEFAULT_DOMAIN<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$CHECK_DEFAULT_DOMAIN\n"
          continue;
       }

       -i $sp_id $INSTALL_SCRIPT {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(9)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }

       -i $sp_id $CREATE_NEW_CONFIGURATION {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(9)"
          if {$do_log_output == 1} {
               puts "(3)press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "$ANSWER_YES\n"
          continue;
       }

       -i $sp_id $FILENAME_FOR_HOSTLIST {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >${host_file_name}<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "${host_file_name}\n"
          continue;
       }
   
       -i $sp_id $FINISHED_ADDING_HOSTS {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "\n"
          continue;
       }
   
       -i $sp_id $FILE_FOR_HOSTLIST {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_YES<(10)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }

          send -i $sp_id "$ANSWER_YES\n"
          continue;
       }

       -i $sp_id $PREVIOUS_SCREEN {
          flush stdout
          flush $CHECK_OUTPUT
          puts $CHECK_OUTPUT "\n -->testsuite: sending >$ANSWER_NO<(10)"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }

          send -i $sp_id "$ANSWER_NO\n"
          continue;
       }
  
       -i $sp_id $ENTER_A_RANGE {
          set myrange [ get_gid_range $CHECK_USER $CHECK_COMMD_PORT]
          puts $CHECK_OUTPUT "\n -->testsuite: sending >${myrange}<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               set anykey [wait_for_enter 1]
          }

          send -i $sp_id "${myrange}\n"
          continue;
       }

       -i $sp_id $MASTER_INSTALLATION_COMPLETE {
          read_install_list
          lappend CORE_INSTALLED $CHECK_CORE_MASTER
          write_install_list
          set_error "0" "install_qmaster - no errors"
          close_spawn_process $id;
          return; 
       }

       -i $sp_id $ENTER_HOSTS {
          if {$hostcount >= [llength $admin_hosts]} {
              puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<(8)"
              send -i $sp_id "\n"
          } else {
             set admin_host [lindex $admin_hosts $hostcount]
             incr hostcount
             puts $CHECK_OUTPUT "\n -->testsuite: sending >${admin_host}<"
             send -i $sp_id "$admin_host\n"
          }
          continue;
       }
   
       -i $sp_id "More" {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >space<"
          if {$do_log_output == 1} {
               puts "press RETURN"
               gets stdin anykey
          }

          send -i $sp_id " "
          continue;
       }

       -i $sp_id "Error:" {
          set_error "-1" "install_qmaster - $expect_out(0,string)"
          close_spawn_process $id;
          return;
       }

       -i $sp_id $NOT_COMPILED_IN_SECURE_MODE {
          set_error "-2" "install_qmaster - sge_qmaster binary is not compiled in secure mode"
          close_spawn_process $id;
          return;
       }

       -i $sp_id "ommand failed*\n" {
          set_error "-1" "install_qmaster - $expect_out(0,string)"
          close_spawn_process $id;
          return;
       }

       -i $sp_id "can't resolve hostname*\n" {
          set_error "-1" "install_qmaster - $expect_out(0,string)"
          close_spawn_process $id; 
          return;
       }     
   
       -i $sp_id "error:\n" {
          set_error "-1" "install_qmaster - $expect_out(0,string)"
          continue;
       }
   
       -i $sp_id $USING_GID_RANGE_HIT_RETURN {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
          if {$do_log_output == 1} {
               puts "-->testsuite: press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "\n"
          continue;
       }

       -i $sp_id $HIT_RETURN_TO_CONTINUE {
          puts $CHECK_OUTPUT "\n -->testsuite: sending >RETURN<"
          if {$do_log_output == 1} {
               puts "-->testsuite: press RETURN"
               set anykey [wait_for_enter 1]
          }
          send -i $sp_id "\n"
          continue;
       }

       -i $sp_id default {
          set_error "-1" "install_qmaster - undefined behaviour: $expect_out(buffer)"
          close_spawn_process $id; 
          return;
       }
    }
  }
}

proc install_shadowd {} {
   global CHECK_OUTPUT

   puts $CHECK_OUTPUT "no shadowd installation for 5.3 system yet"
   set_error "0" "install_shadowd - not yet implemented"
}
