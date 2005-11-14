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

   set catch_result [ catch { eval exec "cat $ts_config(product_root)/inst_sge | grep \"SCRIPT_VERSION\" | cut -d\" -f2" } INST_VERSION ]
   puts $CHECK_OUTPUT "inst_sge version: $INST_VERSION"

   if {! $check_use_installed_system} {
      set feature_install_options ""
      foreach elem $CHECK_SUBMIT_ONLY_HOSTS {
         puts $CHECK_OUTPUT "do a qconf -as $elem ..."
         catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-as $elem" } result
         puts $CHECK_OUTPUT $result
      }
      if { $ts_config(product_feature) == "csp" } {
         set feature_install_options "-csp"
         set my_csp_host_list $ts_config(execd_nodes)
         foreach elem $CHECK_SUBMIT_ONLY_HOSTS {
           lappend my_csp_host_list $elem
         }
         foreach exec_host $my_csp_host_list {
            if { $exec_host == $CHECK_CORE_MASTER } {
               continue;
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
            set result [ start_remote_prog $ts_config(master_host) "root" "cp" "$arguments" ] 
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


      cd "$ts_config(product_root)"

      set prod_type_var "SGE_ROOT"
      set my_timeout 500
      set exit_val 0
      set autoconfig_file $ts_config(product_root)/autoinst_config_$exec_host.conf
     
      write_autoinst_config $autoconfig_file $exec_host 0ß
  
      if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
         set output [start_remote_prog "$exec_host" "root"  "cd" "$$prod_type_var;./install_execd $CHECK_EXECD_INSTALL_OPTIONS $feature_install_options -auto $autoconfig_file -noremote" "exit_val" ]

      } else {
         puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
         set output [start_remote_prog "$exec_host" "$CHECK_USER"  "cd" "$$prod_type_var;./install_execd $CHECK_EXECD_INSTALL_OPTIONS $feature_install_options -auto $autoconfig_file -noremote" "exit_val" ]
      }

      log_user 1
      puts $CHECK_OUTPUT "cd $$prod_type_var;./install_execd $CHECK_EXECD_INSTALL_OPTIONS $feature_install_options -auto $ts_config(product_root)/autoinst_config.conf -noremote"

      #set CHECK_DEBUG_LEVEL 2
      set do_log_output 0 ;# 1 _LOG
      if { $CHECK_DEBUG_LEVEL == 2 } {
         set do_log_output 1
      }

      if { $exit_val == 0 } {
         set_error 0 "ok"
         lappend CORE_INSTALLED $exec_host
         write_install_list
         continue
      } else { 
         set_error "-1" "install failed"
         add_proc_error "install_execd" "-1" "$output"
      }
   }
   return
}


