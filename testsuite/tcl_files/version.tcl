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

proc ts_source {filebase} {
   global ts_config
   global CHECK_OUTPUT

   set ret 1

   # we need a testsuite config before sourcing files
   if {![info exists ts_config] || ![info exists ts_config(gridengine_version)]} {
      add_proc_error "ts_source" -1 "can't source version specific files before knowing the version"
      set ret 0
   } else {
      # read a version independent file first, then the version dependent
      set version $ts_config(gridengine_version)

      set filename "$filebase.tcl"

      if {[file exists $filename]} {
         puts $CHECK_OUTPUT "reading file $filename"
         source $filename
      }

      if { $version != "" } {
         set filename "$filebase.$version.tcl"
         if {[file exists $filename]} {
            puts $CHECK_OUTPUT "reading version specific file $filename"
            source $filename
         }
      }
   }

   return $ret
}

#                                                             max. column:     |
#
#****** control_procedures/resolve_version() ******
#  NAME
#     resolve_version() -- get testsuite internal version number for product 
#
#  SYNOPSIS
#     resolve_version { { internal_number -100 } } 
#
#  FUNCTION
#     This procedure will compare the product version string with known version
#     numbers of the cluster software. A known version number will return a
#     value > 0. The return value is an integer and the test procedures can
#     enable or disable a check procedure by using this number.
#     If an internal version number is given as parameter, a list of 
#     SGE versions mapping to this internal number is returned.
#
#  INPUTS
#     { internal_number -100 } - optional parameter
#                                if set to a integer value > -3 the function
#                                will return a list of corresponding product 
#                                version strings.
#
#  RESULT
#     when internal_number == -100 :
#     ==============================
#
#     -4  - unsupported version
#     -3  - system not running
#     -2  - system not installed
#     -1  - unknown error (testsuite error)
#      0  - version number not set (testsuite error)
#      1  - SGE 5.0.x
#      2  - SGEEE 5.0.x
#      3  - SGE(EE) 6.x
#      ...
#
#     when internal_number != -100 :
#     ==============================
#      
#      List of version strings of the cluster software that match the
#      internal version number of the testsuite.
#
#  KNOWN BUGS
#      A version string should not contain underscores (_); if an internal
#      version number is given to resolve_version, all underscores are mapped
#      to a space.
#
#  SEE ALSO
#     sge_procedures/get_version_info()
#*******************************
#
proc resolve_version { { internal_number -100 } } {

   global CHECK_PRODUCT_VERSION_NUMBER CHECK_PRODUCT_FEATURE CHECK_PRODUCT_ROOT
   global CHECK_PRODUCT_TYPE
   
   if { [ string compare "system not running - run install test first" $CHECK_PRODUCT_VERSION_NUMBER] == 0 } {
      get_version_info
   }
   if { [ string compare "system not installed - run compile option first" $CHECK_PRODUCT_VERSION_NUMBER] == 0 } {
      get_version_info
   }
   if { [ string compare "unknown" $CHECK_PRODUCT_VERSION_NUMBER] == 0 } {
      get_version_info
   }

   set versions(system_not_running_-_run_install_test_first)      -3
   set versions(system_not_installed_-_run_compile_option_first)  -2
   set versions(unknown)                                          -1

   set versions(SGE_5.3)             1
   set versions(SGE_5.3_alpha1)      1
   set versions(SGEEE_5.3)           1
   set versions(SGEEE_5.3_alpha1)    1
   set versions(SGE_6.0_pre)         1
   set versions(SGE_5.3_maintrunc)   2
   set versions(SGEEE_5.3_maintrunc) 2
   set versions(SGE_5.3beta1)        2
   set versions(SGEEE_5.3beta1)      2
   set versions(SGEEE_5.3beta2)      2
   set versions(SGE_5.3beta2)        2
   set versions(SGEEE_5.3beta2_1)    2
   set versions(SGE_5.3beta2_1)      2
   set versions(SGEEE_5.3beta2_2)    2
   set versions(SGE_5.3beta2_2)      2
   set versions(SGEEE_5.3.1beta1)    2
   set versions(SGE_5.3.1beta1)      2
   set versions(SGEEE_5.3.1beta2)    2
   set versions(SGE_5.3.1beta2)      2
   set versions(SGEEE_5.3.1beta3)    2
   set versions(SGE_5.3.1beta3)      2
   set versions(SGEEE_5.3.1beta4)    2
   set versions(SGE_5.3.1beta4)      2
   set versions(SGEEE_5.3.1beta5)    2
   set versions(SGE_5.3.1beta5)      2
   set versions(SGEEE_5.3.1beta6)    2
   set versions(SGE_5.3.1beta6)      2
   set versions(SGEEE_5.3.1beta7)    2
   set versions(SGE_5.3.1beta7)      2
   set versions(SGEEE_5.3.1beta8)    2
   set versions(SGE_5.3.1beta8)      2
   set versions(SGEEE_5.3.1beta9)    2
   set versions(SGE_5.3.1beta9)      2
   set versions(SGEEE_5.3p1)         2
   set versions(SGE_5.3p1)           2
   set versions(SGEEE_5.3p2)         2
   set versions(SGE_5.3p2)           2
   set versions(SGEEE_5.3p3)         2
   set versions(SGE_5.3p3)           2
   set versions(SGEEE_5.3prep4)      2
   set versions(SGE_5.3prep4)        2
   set versions(SGEEE_5.3p4)         2
   set versions(SGE_5.3p4)           2
   set versions(SGEEE_pre6.0_(Maintrunk))    3
   set versions(SGE_pre6.0_(Maintrunk))      3

   if { $internal_number == -100 } {
      if { $CHECK_PRODUCT_VERSION_NUMBER == "" } {
         return 0
      }
      set requested_version [string map {{ } {_}} $CHECK_PRODUCT_VERSION_NUMBER]
      if {[info exists versions($requested_version)] } {
         return $versions($requested_version)
      }   
      add_proc_error "resolve_version" "-1" "Product version \"$CHECK_PRODUCT_VERSION_NUMBER\" not supported"
      return -4
   } else {
      set ret ""
      foreach elem [array names versions] {
         if { $internal_number == $versions($elem) } {
            lappend ret [string map {{_} { }} $elem]
         }
      }
      if { [llength $ret] > 0 } {
         return $ret
      }   
      add_proc_error "resolve_version" "-1" "Internal version number \"$internal_number\" not supported"
      return ""
   }
}




#                                                             max. column:     |
#****** sge_procedures/get_version_info() ******
# 
#  NAME
#     get_version_info -- get version number of the cluster software
#
#  SYNOPSIS
#     get_version_info { } 
#
#  FUNCTION
#     This procedure will return the version string
#
#  RESULT
#     returns the first line of "qconf -help" (this is the version number of 
#     the SGEEE/SGE system).
#
#  SEE ALSO
#     ???/???
#*******************************
proc get_version_info {} {
   global bootstrap
   global CHECK_PRODUCT_VERSION_NUMBER CHECK_PRODUCT_ROOT CHECK_ARCH
   global CHECK_PRODUCT_FEATURE CHECK_PRODUCT_TYPE CHECK_OUTPUT
   global CHECK_CHECKTREE_ROOT
 

   if { [info exists CHECK_PRODUCT_ROOT] != 1 } {
      set CHECK_PRODUCT_VERSION_NUMBER "system not running - run install test first"
      return $CHECK_PRODUCT_VERSION_NUMBER
   }
   
   if { [file isfile "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"] == 1 } {
      set qmaster_running [ catch { 
         eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -sh" 
      } result ]

      catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-help" } result
      set help [ split $result "\n" ] 
      if { ([ string first "fopen" [ lindex $help 0] ] >= 0)        || 
           ([ string first "error" [ lindex $help 0] ] >= 0)        || 
           ([ string first "product_mode" [ lindex $help 0] ] >= 0) ||   
           ($qmaster_running != 0) } {
          set CHECK_PRODUCT_VERSION_NUMBER "system not running - run install test first"
          return $CHECK_PRODUCT_VERSION_NUMBER
      }
      set CHECK_PRODUCT_VERSION_NUMBER [ lindex $help 0]
      if { [ string first "exit" $CHECK_PRODUCT_VERSION_NUMBER ] >= 0 } {
         set CHECK_PRODUCT_VERSION_NUMBER "system not running - run install test first"
      } else {
         set version [resolve_version]
         if {$version < 3} {
            # SGE(EE) 5.x: we have a product mode file
            set product_mode "unknown"
            if { [file isfile $CHECK_PRODUCT_ROOT/default/common/product_mode ] == 1 } {
               set product_mode_file [ open $CHECK_PRODUCT_ROOT/default/common/product_mode "r" ]
               gets $product_mode_file product_mode
               close $product_mode_file
            } else {
               # SGE(EE) 6.x: product mode is in bootstrap file
               set product_mode $bootstrap(product_mode)
            }
            if { $CHECK_PRODUCT_FEATURE == "csp" } {
                if { [ string first "csp" $product_mode ] < 0 } {
                    puts $CHECK_OUTPUT "get_version_info - product feature is not csp ( secure )"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            } else {
                if { [ string first "csp" $product_mode ] >= 0 } {
                    puts $CHECK_OUTPUT "resolve_version - product feature is csp ( secure )"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            }
            if { $CHECK_PRODUCT_TYPE == "sgeee" } {
                if { [ string first "sgeee" $product_mode ] < 0 } {
                    puts $CHECK_OUTPUT "resolve_version - no sgeee system"
                    puts $CHECK_OUTPUT "please remove the file"
                    puts $CHECK_OUTPUT "\n$CHECK_PRODUCT_ROOT/default/common/product_mode"
                    puts $CHECK_OUTPUT "\nif you want to install a new sge system"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            } else {
                if { [ string first "sgeee" $product_mode ] >= 0 } {
                    puts $CHECK_OUTPUT "resolve_version - this is a sgeee system"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            }
         }
      }  
      return $CHECK_PRODUCT_VERSION_NUMBER
   }
   set CHECK_PRODUCT_VERSION_NUMBER "system not installed - run compile option first"
   return $CHECK_PRODUCT_VERSION_NUMBER
}


