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
               continue;
            }
            copy_certificates $shadow_host
         }
      }
   }
 
   foreach shadow_host $CHECK_CORE_SHADOWD {

      puts $CHECK_OUTPUT "installing shadowd on host $shadow_host ($ts_config(product_type) system) ..."
      if { $check_use_installed_system != 0 } {
         set_error "0" "install_shadowd - no need to install shadowd on host \"$shadow_host\" - noinst parameter is set"
         puts "no need to install shadowd on host \"$shadow_host\", noinst parameter is set"
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

      cd "$ts_config(product_root)"

      set prod_type_var "SGE_ROOT"
      set my_timeout 500
      set exit_val 0
  
      if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
         set output [start_remote_prog "$shadow_host" "root"  "cd" "$$prod_type_var;./inst_sge -sm -auto $ts_config(product_root)/autoinst_config.conf" "exit_val" $my_timeout ]
      } else {
         puts $CHECK_OUTPUT "--> install as user $CHECK_USER <--" 
         set output [start_remote_prog "$shadow_host" "$CHECK_USER"  "cd" "$$prod_type_var;./inst_sge -sm -auto $ts_config(product_root)/autoinst_config.conf" "exit_val" $my_timeout ]
      }


      log_user 1
      puts $CHECK_OUTPUT "cd $$prod_type_var;./inst_sge -sm"

      set do_log_output 0 ;# 1 _LOG
      if { $CHECK_DEBUG_LEVEL == 2 } {
         set do_log_output 1
      }

      if { $exit_val == 0 } {
       set_error "0" "ok"
       lappend CORE_INSTALLED $shadow_host
       write_install_list
       return
      } else { 
       set_error "-2" "install failed"
       add_proc_error "install shadowd" "-2" "$output"
       return
      }
   }
}


