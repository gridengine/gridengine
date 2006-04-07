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
   global CHECK_CORE_MASTER CORE_INSTALLED CHECK_OUTPUT 
   global check_use_installed_system CHECK_ADMIN_USER_SYSTEM
   global CHECK_DEBUG_LEVEL CHECK_QMASTER_INSTALL_OPTIONS 
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

   set_error "0" "inst_sge - no errors"
   if {[file isfile "$ts_config(product_root)/inst_sge"] != 1} {
    set_error "-1" "inst_sge - inst_sge file not found"
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

   cd "$ts_config(product_root)"

   set prod_type_var "SGE_ROOT"

   set feature_install_options ""
   if { $ts_config(product_feature) == "csp" } {
      append feature_install_options "-csp"
   }

   set my_timeout 500
   set exit_val 0

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
    set output [start_remote_prog "$CHECK_CORE_MASTER" "root"  "cd" "$$prod_type_var;./install_qmaster $CHECK_QMASTER_INSTALL_OPTIONS $feature_install_options -auto $ts_config(product_root)/autoinst_config.conf" "exit_val" $my_timeout ]
   } else {
    puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
    set output [start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER"  "cd" "$$prod_type_var;./install_qmaster $CHECK_QMASTER_INSTALL_OPTIONS $feature_install_options -auto $ts_config(product_root)/autoinst_config.conf" "exit_val" $my_timeout ]
   }

   log_user 1
   puts $CHECK_OUTPUT "cd $$prod_type_var;./install_qmaster $CHECK_QMASTER_INSTALL_OPTIONS $feature_install_options -auto $ts_config(product_root)/autoinst_config.conf"

   set hostcount 0


   set do_log_output 0;# _LOG
   if { $CHECK_DEBUG_LEVEL == 2 } {
   set do_log_output  1 ;# 1

   }

   if { $exit_val == 0 } {
    set_error "0" "ok"
    lappend CORE_INSTALLED $CHECK_CORE_MASTER
    write_install_list
    return
   } else { 
    set_error "-2" "install failed"
    add_proc_error "install_qmaster" "-2" "$output"
    return
   }

}

proc write_autoinst_config { filename host { do_cleanup 1 } } {

   global ts_config CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT local_execd_spool_set

   set execd_port [expr $ts_config(commd_port) + 1]
   set gid_range [get_gid_range $CHECK_USER $ts_config(commd_port)]

   set bdb_server $ts_config(bdb_server)
   if {$bdb_server == "none"} {
      set db_dir [get_bdb_spooldir $ts_config(master_host) 1]
      # deleting berkeley db spool dir. autoinstall will stop, if
      # bdb spooldir exists.
      if {[file isdirectory "$db_dir"] == 1} {
         delete_directory "$db_dir"
      }
   } else {
      # in this case, the berkeley db rpc server spool dir will be removed,
      # by rpc server install procedure
      set db_dir [get_bdb_spooldir $bdb_server 1]
   }
   puts $CHECK_OUTPUT "db_dir is $db_dir"

   puts $CHECK_OUTPUT "delete file $filename ..."
   file delete $filename
#   wait for remote file deletion ...
   wait_for_remote_file $host $CHECK_USER $filename 60 1 1

   set fdo [open $filename w]

   puts $fdo "SGE_ROOT=\"$ts_config(product_root)\""
   puts $fdo "SGE_QMASTER_PORT=\"$ts_config(commd_port)\""
   puts $fdo "SGE_EXECD_PORT=\"$execd_port\""
   puts $fdo "CELL_NAME=\"$ts_config(cell)\""
   puts $fdo "ADMIN_USER=\"$CHECK_USER\""
   set spooldir [get_local_spool_dir $host qmaster $do_cleanup ]
   if { $spooldir != "" } {
      puts $fdo "QMASTER_SPOOL_DIR=\"$spooldir\""
   } else {
      puts $fdo "QMASTER_SPOOL_DIR=\"$ts_config(product_root)/$ts_config(cell)/spool/qmaster\""
   }
   puts $fdo "EXECD_SPOOL_DIR=\"$ts_config(product_root)/$ts_config(cell)/spool/\""
   puts $fdo "GID_RANGE=\"$gid_range\""
   puts $fdo "SPOOLING_METHOD=\"$ts_config(spooling_method)\""
   puts $fdo "DB_SPOOLING_SERVER=\"$bdb_server\""
   puts $fdo "DB_SPOOLING_DIR=\"$db_dir\""
   puts $fdo "ADMIN_HOST_LIST=\"$ts_config(all_nodes)\""
   puts $fdo "SUBMIT_HOST_LIST=\"$ts_config(all_nodes) $ts_config(submit_only_hosts)\""
   puts $fdo "EXEC_HOST_LIST=\"$ts_config(execd_nodes)\""
   set spooldir [get_local_spool_dir $host execd 0 ]
   if { $spooldir != "" } {
      puts $fdo "EXECD_SPOOL_DIR_LOCAL=\"$spooldir\""
   } else {
      puts $fdo "EXECD_SPOOL_DIR_LOCAL=\"\""
   }
   puts $fdo "HOSTNAME_RESOLVING=\"true\""
   puts $fdo "SHELL_NAME=\"rsh\""
   puts $fdo "COPY_COMMAND=\"rcp\""
   puts $fdo "DEFAULT_DOMAIN=\"none\""
   puts $fdo "ADMIN_MAIL=\"$ts_config(report_mail_to)\""
   puts $fdo "ADD_TO_RC=\"false\""
   puts $fdo "SET_FILE_PERMS=\"true\""
   puts $fdo "RESCHEDULE_JOBS=\"wait\""
   puts $fdo "SCHEDD_CONF=\"1\""
   puts $fdo "SHADOW_HOST=\"$ts_config(shadowd_hosts)\""
   puts $fdo "EXEC_HOST_LIST_RM=\"$ts_config(execd_nodes)\""
   puts $fdo "REMOVE_RC=\"false\""
   puts $fdo "WINDOWS_SUPPORT=\"false\""
   puts $fdo "WIN_ADMIN_NAME=\"Administrator\""
   puts $fdo "CSP_RECREATE=\"true\""
   puts $fdo "CSP_COPY_CERTS=\"true\""
   puts $fdo "CSP_COUNTRY_CODE=\"DE\""
   puts $fdo "CSP_STATE=\"Germany\""
   puts $fdo "CSP_LOCATION=\"Building\""
   puts $fdo "CSP_ORGA=\"Devel\""
   puts $fdo "CSP_ORGA_UNIT=\"Software\""
   puts $fdo "CSP_MAIL_ADDRESS=\"$ts_config(report_mail_to)\""
   close $fdo
   wait_for_remote_file $host $CHECK_USER $filename
}

#                                                             max. column:     |
#****** install_core_system/create_autoinst_config() ******
# 
#  NAME
#     create_autoinst_config -- ??? 
#
#  SYNOPSIS
#     create_autoinst_config { } 
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
proc create_autoinst_config {} {

   global ts_config
   global CHECK_USER check_errstr 
   global CHECK_CORE_MASTER CORE_INSTALLED CHECK_OUTPUT 
   global check_use_installed_system CHECK_ADMIN_USER_SYSTEM
   global CHECK_DEBUG_LEVEL CHECK_QMASTER_INSTALL_OPTIONS 
   global CHECK_PROTOCOL_DIR

   if { [file isfile "$ts_config(product_root)/autoinst_config.conf"] == 1} {
      set catch_result [ catch { eval exec "rm -f $ts_config(product_root)/autoinst_config.conf" } ]
   }

   puts $CHECK_OUTPUT "creating automatic install config file ..."
   write_autoinst_config  $ts_config(product_root)/autoinst_config.conf $ts_config(master_host) 1
   puts $CHECK_OUTPUT "automatic install config file successfully created ..."
   set_error "0" "inst_sge - no errors"

} 
