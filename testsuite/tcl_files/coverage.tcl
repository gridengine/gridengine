#!/vol2/TCL_TK/glinux/bin/expect
# expect script 
# test SGE/SGEEE System
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

#****** coverage/coverage_enabled() ********************************************
#  NAME
#     coverage_enabled() -- code coverage enabled?
#
#  SYNOPSIS
#     coverage_enabled { } 
#
#  FUNCTION
#     Returns if code coverage is enabled for the current testsuite run.
#
#     One or multiple code coverage methods can be enabled at a time, 
#     "tcov", or "insure" for C/C++ code coverage analysis, 
#     or "emma" for Java code coverage analyis.
#
#     The procedure can check if any coverage method is enabled (parameter
#     method == ""), or a specific one.
#
#  INPUTS
#     {method ""} - coverage method to look for, or "" (default) to check if
#                   any coverage analyis is done
#
#  RESULT
#     0 - if coverage analysis is disabled
#     1 - if coverage analysis is enabled
#*******************************************************************************
proc coverage_enabled {{method ""}} {
   global CHECK_COVERAGE

   set ret 0

   if {$method == ""} {
      if {$CHECK_COVERAGE != {}} {
         set ret 1
      }
   } else {
      if {[lsearch -exact $CHECK_COVERAGE $method] >= 0} {
         set ret 1
      }
   }

   return $ret
}

#****** coverage/coverage_initialize() *****************************************
#  NAME
#     coverage_initialize() -- initialize code coverage analysis
#
#  SYNOPSIS
#     coverage_initialize { } 
#
#  FUNCTION
#     This function is called during testsuite setup.
#     It sets up the required environment (e.g. directories)
#     for code coverage analysis.
#
#  INPUTS
#     {clean 0} - shall the coverage directories be deleted and reinitialized?
#
#  SEE ALSO
#     coverage/insure_initialize()
#     coverage/tcov_initialize()
#*******************************************************************************
proc coverage_initialize {{clean 0}} {
   global CHECK_COVERAGE

   foreach cov $CHECK_COVERAGE {
      set procname "${cov}_initialize"
      if {[info procs $procname] != {}} {
         $procname $clean
      }
   }
}

#****** coverage/coverage_per_process_setup() **********************************
#  NAME
#     coverage_per_process_setup() -- setup process environment
#
#  SYNOPSIS
#     coverage_per_process_setup { host user env_var } 
#
#  FUNCTION
#     Sets up special environment required for starting a process
#     with code coverage testing (e.g. environment variables).
#
#  INPUTS
#     host    - host where the process will be started
#     user    - user who will run the process
#     env_var - name of a TCL array holding environment variables that will
#               be set in the processes environment
#
#  SEE ALSO
#     coverage/tcov_per_process_setup()
#*******************************************************************************
proc coverage_per_process_setup {host user env_var} {
   global CHECK_COVERAGE
  
   foreach cov $CHECK_COVERAGE {
      set procname "${cov}_per_process_setup"
      if {[info procs $procname] != {}} {
         $procname $host $user env
      }
   }
}

#****** coverage/coverage_join_dirs() ******************************************
#  NAME
#     coverage_join_dirs() -- join host local coverage profile directories
#
#  SYNOPSIS
#     coverage_join_dirs { } 
#
#  FUNCTION
#     The coverage profiles are stored locally on the hosts being part of
#     a test cluster.
#     This function is called to copy the host local files to a shared
#     directory.
#     If necessary (depending on profiling method) multiple profiles are 
#     joined into a single one.
#
#  SEE ALSO
#     coverage/insure_join_dirs()
#     coverage/tcov_join_dirs()
#*******************************************************************************
proc coverage_join_dirs {} {
   global CHECK_COVERAGE

   foreach cov $CHECK_COVERAGE {
      set procname "${cov}_join_dirs"
      if {[info procs $procname] != {}} {
         $procname
      }
   }
}

#****** coverage/coverage_compute_coverage() ***********************************
#  NAME
#     coverage_compute_coverage() -- compute code coverage
#
#  SYNOPSIS
#     coverage_compute_coverage { } 
#
#  FUNCTION
#     This function does code coverage analysis.
#     A short summary is printed, and a detailed HTML/Text
#     document is generaged.
#
#  SEE ALSO
#     coverage/insure_compute_coverage()
#     coverage/tcov_compute_coverage()
#*******************************************************************************
proc coverage_compute_coverage {} {
   global CHECK_COVERAGE

   foreach cov $CHECK_COVERAGE {
      set procname "${cov}_compute_coverage"
      if {[info procs $procname] != {}} {
         $procname
      }
   }
}

#****** coverage/coverage_analyis() ********************************************
#  NAME
#     coverage_analyis() -- do coverage analysis
#
#  SYNOPSIS
#     coverage_analyis { } 
#
#  FUNCTION
#     This function is called (e.g. from testsuite menu) to generate the
#     code coverage analysis.
#
#     Host local coverage profiles are joined in a shared location
#     and the analysis and reporting is done.
#
#  SEE ALSO
#     coverage/coverage_join_dirs()
#     coverage/coverage_compute_coverage()
#*******************************************************************************
proc coverage_analyis {} {
   global CHECK_OUTPUT
   global CHECK_COVERAGE

   if {$CHECK_COVERAGE == "none"} {
      puts $CHECK_OUTPUT "No coverage information available."
      puts $CHECK_OUTPUT "To gather coverage information, please do"
      puts $CHECK_OUTPUT "  o compile with -cov"
      puts $CHECK_OUTPUT "  o install the binaries"
      puts $CHECK_OUTPUT "  o call testsuite with the coverage and coverage_dir options"
      puts $CHECK_OUTPUT "  o run testsuite installation and checks"
      puts $CHECK_OUTPUT "  o call this menu item"
      return
   }

   # gather and join coverage files
   coverage_join_dirs

   # compute coverage information 
   coverage_compute_coverage
}

#****** coverage/insure_get_local_basedir() ************************************
#  NAME
#     insure_get_local_basedir() -- get host local directory
#
#  SYNOPSIS
#     insure_get_local_basedir { } 
#
#  FUNCTION
#     Returns a unique host local directory for the storage of 
#     code coverage profiles.
#
#  RESULT
#     A directory name.
#     The current implementation returns a directory 
#     /tmp/insure/<qmaster commd port number>.
#*******************************************************************************
proc insure_get_local_basedir {} {
   global ts_config

   return "/tmp/insure/$ts_config(commd_port)"
}

#****** coverage/insure_initialize() *******************************************
#  NAME
#     insure_initialize() -- initialize for insure coverage profiling
#
#  SYNOPSIS
#     insure_initialize { } 
#
#  FUNCTION
#     Prepares the environment required for running code coverage tests 
#     with insure:
#        o create local profile directories on all hosts
#        o create insure settings file (.psrc) for all cluster users
#
#  SEE ALSO
#     coverage/coverage_initialize()
#*******************************************************************************
proc insure_initialize {{clean 0}} {
   global ts_config CHECK_OUTPUT
   global CHECK_COVERAGE_DIR CHECK_HOST

   if { [have_root_passwd] == -1 } {
      puts $CHECK_OUTPUT "need root access ..."
      set_root_passwd
   }

   # create a local log directory on all hosts
   set basedir [insure_get_local_basedir]
   puts -nonewline $CHECK_OUTPUT "creating local log directories on host" ; flush $CHECK_OUTPUT
   set hosts [host_conf_get_cluster_hosts]
   set users [user_conf_get_cluster_users]
   foreach host $hosts {
      puts -nonewline " $host" ; flush $CHECK_OUTPUT
      start_remote_prog $host "root" "$ts_config(testsuite_root_dir)/scripts/insure_create_log_dirs.sh" "$clean $basedir $users"
   }
   puts $CHECK_OUTPUT " done"

   # create .psrc file as temporary file
   # and copy .psrc file into users home directories
   puts $CHECK_OUTPUT "installing .psrc files for all users"
   foreach user $users {
      set tmp_psrc [get_tmp_file_name]
      set f [open $tmp_psrc "w"]
      puts $f "# .psrc file created by Gridengine testsuite"
      puts $f "insure++.ReportFile insra"
      puts $f "insure++.suppress PARM_NULL"
      puts $f "insure++.suppress BAD_INTERFACE"
      puts $f "insure++.compiler_default cpp"
      puts $f "insure++.demangle off"
      puts $f "insure++.leak_search off"
      puts $f "insure++.leak_sweep off"
      puts $f "insure++.checking_uninit off"
      puts $f "insure++.ignore_wild off"
      puts $f "insure++.temp_directory /tmp"
      puts $f "#insure++.coverage_boolean on"
      puts $f "insure++.coverage_only on"
      puts $f "insure++.coverage_map_data on"
      # we might add %a (architecture) to the following option
      puts $f "insure++.coverage_map_file ${CHECK_COVERAGE_DIR}/tca.map"
      puts $f "insure++.coverage_log_data on"
      puts $f "insure++.coverage_overwrite off"
      # we might want to add the hostname as directory to the following option.
      # We should also consider if storing the log files on a local filesystem
      # could significantly speed up testsuite execution
      puts $f "insure++.coverage_log_file ${basedir}/${user}/tca.%v.log"
      puts $f "insure++.coverage_banner off"
      puts $f "insure++.report_banner off"
      puts $f "insure++.threaded_runtime on"
      puts $f "insure++.avoidExternDecls *"
      close $f

      # the user might have local home directory,
      # so copy his .psrc to every host
      # and we have to create the local log directory on every host
      puts -nonewline $CHECK_OUTPUT "-> user $user on host" ; flush $CHECK_OUTPUT
      foreach host [host_conf_get_cluster_hosts] {
         puts -nonewline $CHECK_OUTPUT " $host" ; flush $CHECK_OUTPUT
         start_remote_prog $host $user "cp" "$tmp_psrc \$HOME/.psrc"
      }
      puts $CHECK_OUTPUT " done"
   }
}

proc insure_join_dirs {} {
   global ts_config CHECK_OUTPUT
   global CHECK_COVERAGE_DIR CHECK_HOST

   if { [have_root_passwd] == -1 } {
      puts $CHECK_OUTPUT "need root access ..."
      set_root_passwd
   }

   # copy from local logdir (basedir) to CHECK_COVERAGE_DIR/$host
   set basedir [insure_get_local_basedir]
   puts -nonewline $CHECK_OUTPUT "copying local log directories from host" ; flush $CHECK_OUTPUT
   set hosts [host_conf_get_cluster_hosts]
   foreach host $hosts {
      puts -nonewline " $host" ; flush $CHECK_OUTPUT
      start_remote_prog $host "root" "$ts_config(testsuite_root_dir)/scripts/insure_join_log_dirs.sh" "$basedir ${CHECK_COVERAGE_DIR}/${host}" prg_exit_state 600
   }
   puts $CHECK_OUTPUT " done"
}

proc insure_compute_coverage {} {


# source code with coverage for a certain file:
# tca -ds -ct -fF sge_dstring.c /cod_home/joga/sys/tca/*/*/tca*.log
#
# coverage summary per file
# tca -dS -fF sge_dstring.c /cod_home/joga/sys/tca/*/*/tca*.log

}

#****** coverage/tcov_get_local_basedir() ************************************
#  NAME
#     tcov_get_local_basedir() -- get host local directory
#
#  SYNOPSIS
#     tcov_get_local_basedir { } 
#
#  FUNCTION
#     Returns a unique host local directory for the storage of 
#     code coverage profiles.
#
#  RESULT
#     A directory name.
#     The current implementation returns a directory 
#     /tmp/tcov<qmaster commd port number>.
#*******************************************************************************
proc tcov_get_local_basedir {} {
   global ts_config

   return "/tmp/tcov/$ts_config(commd_port)"
}

#****** coverage/tcov_initialize() *******************************************
#  NAME
#     tcov_initialize() -- initialize for tcov coverage profiling
#
#  SYNOPSIS
#     tcov_initialize { } 
#
#  FUNCTION
#     Prepares the environment required for running code coverage tests 
#     with tcov:
#        o create local profile directories on all hosts
#        o setup environment for processes called directly from expect
#
#  SEE ALSO
#     coverage/coverage_initialize()
#     coverage/tcov_per_process_setup()
#*******************************************************************************
proc tcov_initialize {{clean 0}} {
   global ts_config CHECK_OUTPUT
   global CHECK_COVERAGE_DIR CHECK_HOST CHECK_USER
   global env

   if { [have_root_passwd] == -1 } {
      puts $CHECK_OUTPUT "need root access ..."
      set_root_passwd
   }

   # create a local log directory on all hosts
   set basedir [tcov_get_local_basedir]
   puts -nonewline $CHECK_OUTPUT "creating local log directories on host" ; flush $CHECK_OUTPUT
   set hosts [host_conf_get_cluster_hosts]
   set users [user_conf_get_cluster_users]
   foreach host $hosts {
      puts -nonewline " $host" ; flush $CHECK_OUTPUT
      start_remote_prog $host "root" "$ts_config(testsuite_root_dir)/scripts/tcov_create_log_dirs.sh" "$clean $basedir $users"
   }
   puts $CHECK_OUTPUT " done"

   # setup the environment to use the profile directory
   # for processes started directly by expect (eval exec)
   tcov_per_process_setup $CHECK_HOST $CHECK_USER env
}

#****** coverage/tcov_per_process_setup() **********************************
#  NAME
#     tcov_per_process_setup() -- setup process environment
#
#  SYNOPSIS
#     tcov_per_process_setup { host user env_var } 
#
#  FUNCTION
#     Sets up special environment required for starting a process
#     with code tcov coverage testing (e.g. environment variables).
#
#     Sets the environment variable SUN_PROFDATA_DIR to point to the
#     host local, user specific profile directory.
#
#  INPUTS
#     host    - host where the process will be started
#     user    - user who will run the process
#     env_var - name of a TCL array holding environment variables that will
#               be set in the processes environment
#
#  SEE ALSO
#     coverage/coverage_per_process_setup()
#*******************************************************************************
proc tcov_per_process_setup {host user env_var} {
   upvar $env_var env

   set basedir [tcov_get_local_basedir]
   set env(SUN_PROFDATA_DIR) "${basedir}/${user}"
}

#****** coverage/tcov_join_dirs() ******************************************
#  NAME
#     tcov_join_dirs() -- join host local coverage profile directories
#
#  SYNOPSIS
#     tcov_join_dirs { } 
#
#  FUNCTION
#     The coverage profiles are stored locally on the hosts being part of
#     a test cluster.
#     This function is called to copy the host local files to a shared
#     directory.
#
#     In addition, all coverage profiles (files named "tcovd") are joined into
#     a single coverage file, as tcov itself does not analyze (join) 
#     multiple profiles, as for example insure does.
#
#  SEE ALSO
#     coverage/coverage_join_dirs()
#*******************************************************************************
proc tcov_join_dirs {} {
   global ts_config CHECK_OUTPUT
   global CHECK_COVERAGE_DIR CHECK_HOST

   if { [have_root_passwd] == -1 } {
      puts $CHECK_OUTPUT "need root access ..."
      set_root_passwd
   }

   # copy from local logdir (basedir) to CHECK_COVERAGE_DIR/$host
   set basedir [tcov_get_local_basedir]
   puts -nonewline $CHECK_OUTPUT "copying local log directories from host" ; flush $CHECK_OUTPUT
   set hosts [host_conf_get_cluster_hosts]
   foreach host $hosts {
      puts -nonewline " $host" ; flush $CHECK_OUTPUT
      start_remote_prog $host "root" "$ts_config(testsuite_root_dir)/scripts/tcov_join_log_dirs.sh" "$basedir ${CHECK_COVERAGE_DIR}/${host}" prg_exit_state 600
   }
   puts $CHECK_OUTPUT " done"

   # join coverage files into one
   puts $CHECK_OUTPUT "joining all coverage profiles into a single one"
   cd $CHECK_COVERAGE_DIR
   set profiles {}
   foreach host $hosts {
      set host_profiles [glob -nocomplain "${host}/*/*/tcovd"]
      foreach profile $host_profiles {
         lappend profiles $profile
      }
   }
   puts -nonewline $CHECK_OUTPUT "parsing [llength $profiles] profiles " ; flush $CHECK_OUTPUT
   foreach profile $profiles {
      tcov_parse_coverage_file $profile
   }
   puts $CHECK_OUTPUT " done"
   puts -nonewline $CHECK_OUTPUT "dumping joined profile" ; flush $CHECK_OUTPUT
   tcov_dump_coverage "total.profile"
   puts $CHECK_OUTPUT " ... done"
}

#****** coverage/tcov_get_object_array_name() **********************************
#  NAME
#     tcov_get_object_array_name() -- return unique variable name
#
#  SYNOPSIS
#     tcov_get_object_array_name { } 
#
#  FUNCTION
#     Returns a unique name for a TCL array.
#
#  RESULT
#     unique name
#
#  NOTES
#     No public interface. Only used internally be tcov coverage analyis.
#*******************************************************************************
proc tcov_get_object_array_name {} {
   global tcov_object_count

   if {![info exists tcov_object_count]} {
      set tcov_object_count 0
   }

   incr tcov_object_count
   return "tcov_object_${tcov_object_count}"
}

#****** coverage/tcov_parse_coverage_file() ************************************
#  NAME
#     tcov_parse_coverage_file() -- parse a tcov coverage profile
#
#  SYNOPSIS
#     tcov_parse_coverage_file { filename } 
#
#  FUNCTION
#     Parses a tcov coverage profile and puts all information into 
#     data structures.
#        o tcov_objects
#          TCL array holding information about all specific coverage object.
#          There is one single tcov_objects structure.
#
#        o tcov_object_<unique number>
#          TCL array holding coverage information for a single source code file.
#          There is one such object per source code file.
#
#  INPUTS
#     filename - name of the profile
#
#  NOTES
#     No public interface. Only used internally be tcov coverage analyis.
#*******************************************************************************
proc tcov_parse_coverage_file {filename} {
   global CHECK_OUTPUT
   global tcov_objects

   # open the coverage file
   puts -nonewline $CHECK_OUTPUT "." ; flush $CHECK_OUTPUT
   #puts $filename
   set f [open $filename "r"]

   # parse the coverage file
   while {[gets $f line] >= 0} {
      switch -glob $line {
         "OBJFILE:*" {
            set object_name [lindex $line 1]
            if {![info exists tcov_objects($object_name)]} {
               set tcov_objects($object_name) [tcov_get_object_array_name]
            }
            #puts "$object_name $tcov_objects($object_name)"
            upvar $tcov_objects($object_name) obj
         }
         "TIMESTAMP:*" {
            set obj(timestamp) [lrange $line 1 end]
         }

         "SRCFILE:*" {
            set obj(srcfile) [lindex $line 1]
         }

         default {
            if {[string is space [string range $line 0 1]]} {
               set line [string trim $line]
               set block [lindex $line 0]
               set count [lindex $line 1]
               if {[info exists obj($block)]} {
                  incr obj($block) $count
               } else {
                  set obj($block) $count
                  lappend obj(index) $block
               }
            }
         }
      }
   }

   # cleanup
   close $f
}

#****** coverage/tcov_dump_coverage() ******************************************
#  NAME
#     tcov_dump_coverage() -- dump code coverage information to file
#
#  SYNOPSIS
#     tcov_dump_coverage { dirname } 
#
#  FUNCTION
#     This function dumps code coverage information from internal data structures
#     filled from individual coverage profiles (by calling tcov_parse_coverage_file)
#     into a new profile.
#
#  INPUTS
#     dirname - name of coverage profile (which is a directory)
#
#  NOTES
#     No public interface. Only used internally be tcov coverage analyis.
#
#  SEE ALSO
#     coverage/tcov_parse_coverage_file()
#*******************************************************************************
proc tcov_dump_coverage {dirname} {
   global tcov_objects

   if {![file isdirectory $dirname]} {
      file mkdir $dirname
   }

   set filename "$dirname/tcovd"
   set f [open $filename w]
   puts $f "TCOV-DATA-FILE-VERSION: 2.0"

   foreach object_name [array names tcov_objects] {
      puts $f "OBJFILE: $object_name"
      upvar $tcov_objects($object_name) obj
      puts $f "TIMESTAMP: $obj(timestamp)"

      if {[info exists obj(srcfile)]} {
         puts $f "SRCFILE: $obj(srcfile)"

         foreach block [lsort -integer $obj(index)] {
            puts $f "\t\t$block\t$obj($block)"
         }
      }

      unset obj
   }

   close $f
   unset tcov_objects
}

#****** coverage/tcov_compute_coverage() ***************************************
#  NAME
#     tcov_compute_coverage() -- generate coverage report
#
#  SYNOPSIS
#     tcov_compute_coverage { } 
#
#  FUNCTION
#     Generates a report from tcov coverage information.
#     For each source code file, tcov is run on a joined profile.
#     Coverage information is summed up per directory.
#     A HTML report is generated containing coverage metrics per directory
#     and per source code file, with links to the tcov output per source
#     code file.
#*******************************************************************************
proc tcov_compute_coverage {} {
   global ts_config
   global CHECK_OUTPUT CHECK_PROTOCOL_DIR

   cd $ts_config(source_dir)

   set target_dirs "./clients ./common ./daemons ./libs ./utilbin"
   set target_files "./3rdparty/qmake/remote-sge.c"

   set result(index) {}
   tcov_recursive_coverage "." target_dirs target_files result

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "total blocks:    $result(.,blocks)"
   puts $CHECK_OUTPUT "blocks executed: $result(.,blocks_executed)"

   if {$result(.,blocks) > 0} {
      set coverage [expr $result(.,blocks_executed) * 100.0 / $result(.,blocks)]
      set coverage_text [format "%3.0f" $coverage]
      puts $CHECK_OUTPUT "coverage:        $coverage_text %"
   }

   set html_body [create_html_text "Code coverage"]
   append html_body [create_html_text "$ts_config(source_cvs_release)"]
   append html_body [create_html_text [clock format [clock seconds]]]

   set html_table(1,BGCOLOR) "#3366FF"
   set html_table(1,FNCOLOR) "#66FFFF"
   set html_table(COLS) 4
   set html_table(1,1) "File"
   set html_table(1,2) "Blocks"
   set html_table(1,3) "Executed"
   set html_table(1,4) "Coverage \[%\]"

   set row 1
   foreach file [lsort $result(index)] {
      incr row 1
      if {$result($file,is_node)} {
         set html_table($row,1) $file
      } else {
         set html_table($row,1) [create_html_link $file "${file}.txt"]
      }
      set html_table($row,2) $result($file,blocks)
      set html_table($row,3) $result($file,blocks_executed)
      set coverage 0
      if {$result($file,blocks) > 0} {
         set coverage [expr $result($file,blocks_executed) * 100.0 / $result($file,blocks)]
      }
      set html_table($row,4) [format "%3.0f" $coverage]
      set html_table($row,FNCOLOR) "#000000"
      if {$coverage < 30} {
         set html_table($row,BGCOLOR) "#FF0000"
      } else {
         if {$coverage < 70} {
            set html_table($row,BGCOLOR) "#FFFF00"
         } else {
            set html_table($row,BGCOLOR) "#33FF33"
         }
      }
   }

   set html_table(ROWS) $row
   append html_body [create_html_table html_table]
   generate_html_file "$CHECK_PROTOCOL_DIR/coverage/index.html" "Code Coverage Analysis" $html_body
}

#****** coverage/tcov_recursive_coverage() *************************************
#  NAME
#     tcov_recursive_coverage() -- traverse source tree and call tcov
#
#  SYNOPSIS
#     tcov_recursive_coverage { node subdirs_var files_var result_var } 
#
#  FUNCTION
#     Recursively traverses the source code tree.
#     For all source code files (leafs), the tcov utility is called to generate
#     a report for this file.
#
#  INPUTS
#     node        - current tree node
#     subdirs_var - child nodes of the current node
#     files_var   - leafs under the current node
#     result_var  - name of a TCL array to contain the results
#*******************************************************************************
proc tcov_recursive_coverage {node subdirs_var files_var result_var} {
   global CHECK_OUTPUT

   upvar $subdirs_var subdirs
   upvar $files_var files
   upvar $result_var result

   # initialize node data
   lappend result(index) $node
   set result($node,is_node) 1
   set result($node,blocks) 0
   set result($node,blocks_executed) 0

   # recursively descend tree
   foreach dir $subdirs {
      if {[file tail $dir] != "CVS"} {
         set directories [glob -directory $dir -nocomplain -types d -- *]
         set sourcefiles [glob -directory $dir -nocomplain -types f -- *.c]
         tcov_recursive_coverage $dir directories sourcefiles result

         incr result($node,blocks) $result($dir,blocks)
         incr result($node,blocks_executed) $result($dir,blocks_executed)
      }
   }

   # do coverage analysis for the leaf nodes
   foreach file $files {
      lappend result(index) $file
      set result($file,is_node) 0
      tcov_call_tcov $file result

      incr result($node,blocks) $result($file,blocks)
      incr result($node,blocks_executed) $result($file,blocks_executed)
   }
}

#****** coverage/tcov_call_tcov() **********************************************
#  NAME
#     tcov_call_tcov() -- run tcov utility on a single file
#
#  SYNOPSIS
#     tcov_call_tcov { file result_var } 
#
#  FUNCTION
#     Calls the tcov utility for a single source code file.
#     Store results.
#
#  INPUTS
#     file       - name of source code file
#     result_var - name of TCL array to store the coverage information
#*******************************************************************************
proc tcov_call_tcov {file result_var} {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global CHECK_COVERAGE CHECK_COVERAGE_DIR CHECK_PROTOCOL_DIR

   upvar $result_var result

   set result($file,error) 0
   set result($file,blocks) 0
   set result($file,blocks_executed) 0

   set profile "$CHECK_COVERAGE_DIR/total.profile"
   set tcovfile "$CHECK_PROTOCOL_DIR/coverage/${file}.txt"
   set tcovdir [file dirname $tcovfile]
   if {![file isdirectory $tcovdir]} {
      file mkdir $tcovdir
   }
   if {[file exists $tcovfile]} {
      file delete $tcovfile
   }
   set CHECK_COVERAGE "none"
   set output [start_remote_prog $CHECK_HOST $CHECK_USER "tcov" "-x $profile -o $tcovfile $file"]
   set CHECK_COVERAGE "tcov"
   if {$prg_exit_state == 0} {
      puts -nonewline $CHECK_OUTPUT "+" ; flush $CHECK_OUTPUT
      set f [open $tcovfile "r"]
      while {[gets $f line] >= 0} {
         switch -glob -- $line {
            "*Basic blocks in this file*" {
               set result($file,blocks) [lindex [string trim $line] 0]
            }
            "*Basic blocks executed*" {
               set result($file,blocks_executed) [lindex [string trim $line] 0]
            }
         }
      }
      close $f
   } else {
      puts -nonewline $CHECK_OUTPUT "-" ; flush $CHECK_OUTPUT
      set result($file,error) 1
      set f [open $tcovfile "a+"]
      puts $f "================================================================================"
      puts $f "------------------------- tcov error output ------------------------------------"
      puts $f $output
      puts $f "================================================================================"
      close $f
   }
}
