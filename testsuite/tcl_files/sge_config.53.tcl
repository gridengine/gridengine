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

proc bootstrap_sge_config {} {
   global sge_config ts_config
   global CHECK_USER CHECK_DEFAULT_DOMAIN
   global CHECK_PRODUCT_ROOT CHECK_OUTPUT

   # start from scratch
   if [info exists sge_config] {
      unset sge_config
   }

   # read SGE configuration file
   set bootstrap_file "$CHECK_PRODUCT_ROOT/$ts_config(cell)/common/configuration"
   if {[file exists $bootstrap_file]} {
      puts $CHECK_OUTPUT "reading configuration file $bootstrap_file"
      set f [open $bootstrap_file r]
      while {[gets $f line] > 0} {
         if {[string range $line 0 0] != "#"} {
            set name [lindex $line 0]
            set value [lrange $line 1 end]
            set sge_config($name) "$value"
            #puts $CHECK_OUTPUT "read $name: $value"
         } else {
            #puts $CHECK_OUTPUT "skipping comment"
         }
      }
   } else {   
      puts $CHECK_OUTPUT "initializing sge_config from default values"
      set sge_config(admin_user)        "$CHECK_USER"
      set sge_config(default_domain)    "$CHECK_DEFAULT_DOMAIN"
      set sge_config(ignore_fqdn)       "true"
      set sge_config(spooling_method)   "unknown"
      set sge_config(spooling_lib)      "unknown"
      set sge_config(spooling_params)   "unknown"
      set sge_config(binary_path)       "$CHECK_PRODUCT_ROOT/bin"
      set sge_config(qmaster_spool_dir) "$CHECK_PRODUCT_ROOT/$ts_config(cell)/spool/qmaster"
      set sge_config(product_mode)      "$ts_config(product_type)"
   }
}

