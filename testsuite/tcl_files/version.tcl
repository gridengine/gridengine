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
#
#****** version/ts_source() ******
#  NAME
#     ts_source() -- get testsuite internal version number for product 
#
#  SYNOPSIS
#     ts_source {filebase {extension tcl}} 
#
#  FUNCTION
#     This function sources a tclfile named by filebase and extension.
#     It will first source a version independent file (if it exists) and
#     then a version dependent file.
#
#     It will check if the following files exist, and source them:
#        $filebase.$extension
#        $filebase.$ts_config(gridengine_version).$extension
#
#  INPUTS
#     filebase  - filename without extension, e.g. tcl_files/version
#     extension - extension, e.g. "tcl" or "ext", default "tcl"
#
#  RESULT
#     1 on success, else 0
#
#  SEE ALSO
#*******************************
#
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
   global ts_config
   global sge_config
   global CHECK_PRODUCT_VERSION_NUMBER CHECK_PRODUCT_ROOT CHECK_ARCH
   global CHECK_PRODUCT_TYPE CHECK_OUTPUT
   global CHECK_CHECKTREE_ROOT
 

   if { [info exists CHECK_PRODUCT_ROOT] != 1 } {
      set CHECK_PRODUCT_VERSION_NUMBER "system not running - run install test first"
      return $CHECK_PRODUCT_VERSION_NUMBER
   }
   
   if { [file isfile "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"] } {
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
         if {$ts_config(gridengine_version) == 53} {
            # SGE(EE) 5.x: we have a product mode file
            set product_mode "unknown"
            if { [file isfile $CHECK_PRODUCT_ROOT/default/common/product_mode ] == 1 } {
               set product_mode_file [ open $CHECK_PRODUCT_ROOT/default/common/product_mode "r" ]
               gets $product_mode_file product_mode
               close $product_mode_file
            } else {
               # SGE(EE) 6.x: product mode is in bootstrap file
               set product_mode $sge_config(product_mode)
            }
            if { $ts_config(product_feature) == "csp" } {
                if { [ string first "csp" $product_mode ] < 0 } {
                    puts $CHECK_OUTPUT "get_version_info - product feature is not csp ( secure )"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            } else {
                if { [ string first "csp" $product_mode ] >= 0 } {
                    puts $CHECK_OUTPUT "get_version_info - product feature is csp ( secure )"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            }
            if { $CHECK_PRODUCT_TYPE == "sgeee" } {
                if { [ string first "sgeee" $product_mode ] < 0 } {
                    puts $CHECK_OUTPUT "get_version_info - no sgeee system"
                    puts $CHECK_OUTPUT "please remove the file"
                    puts $CHECK_OUTPUT "\n$CHECK_PRODUCT_ROOT/default/common/product_mode"
                    puts $CHECK_OUTPUT "\nif you want to install a new sge system"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            } else {
                if { [ string first "sgeee" $product_mode ] >= 0 } {
                    puts $CHECK_OUTPUT "get_version_info - this is a sgeee system"
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


