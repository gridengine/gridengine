#!/vol2/TCL_TK/glinux/bin/tclsh
# expect script 
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

global module_name
set module_name "sge_procedures.tcl"

# procedures
#                                                             max. column:     |
#****** sge_procedures/test() ******
# 
#  NAME
#     test -- ??? 
#
#  SYNOPSIS
#     test { m p } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     m - ??? 
#     p - ??? 
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
proc test {m p} {
   puts "test $m $p"
}

#****** sge_procedures/assign_queues_with_pe_object() **************************
#  NAME
#     assign_queues_with_pe_object() -- setup queue <-> pe connection
#
#  SYNOPSIS
#     assign_queues_with_pe_object { queue_list pe_obj } 
#
#  FUNCTION
#     This procedure will setup the queue - pe connections.
#
#  INPUTS
#     queue_list - queue list for the pe object
#     pe_obj     - name of pe object
#
#  SEE ALSO
#     sge_procedures/assign_queues_with_ckpt_object()
#*******************************************************************************
proc assign_queues_with_pe_object { queue_list pe_obj } {
   global CHECK_OUTPUT
   if { [get_pe_ckpt_version] > 0 } {
      foreach queue $queue_list {
         get_queue $queue org_val
         if { [string match "NONE" $org_val(pe_list)] ||
              [string match "none" $org_val(pe_list)] } {
            puts $CHECK_OUTPUT "overwriting NONE value for pe_list in queue $queue"
            set new_val(pe_list) $pe_obj
         } else {
            puts $CHECK_OUTPUT "adding new pe object to pe_list in queue $queue"
            set new_val(pe_list) "$org_val(pe_list),$pe_obj"
         }
         set_queue $queue new_val
      }
   } else {
      set q_list ""
      foreach elem $queue_list {
         if [ string compare $q_list "" ] {
            set q_list "$q_list,$elem"
         } else {
            set q_list "$elem"
         }
      }
      set my_change(queue_list) $q_list
      set_pe $pe_obj my_change
   }
}

#****** sge_procedures/assign_queues_with_ckpt_object() ************************
#  NAME
#     assign_queues_with_ckpt_object() -- setup queue <-> ckpt connection
#
#  SYNOPSIS
#     assign_queues_with_ckpt_object { queue_list ckpt_obj } 
#
#  FUNCTION
#     This procedure will setup the queue - ckpt connections.
#
#  INPUTS
#     queue_list - queue list for the ckpt object
#     ckpt_obj   - name of ckpt object
#
#  SEE ALSO
#     sge_procedures/assign_queues_with_pe_object()
#*******************************************************************************
proc assign_queues_with_ckpt_object { queue_list ckpt_obj } {
   global CHECK_OUTPUT
   if { [get_pe_ckpt_version] > 0 } {
      foreach queue $queue_list {
         get_queue $queue org_val
         if { [string match "NONE" $org_val(ckpt_list)] ||
              [string match "none" $org_val(ckpt_list)] } {
            puts $CHECK_OUTPUT "overwriting NONE value for ckpt_list in queue $queue"
            set new_val(ckpt_list) $ckpt_obj
         } else {
            puts $CHECK_OUTPUT "adding new ckpt object to ckpt_list in queue $queue"
            set new_val(ckpt_list) "$org_val(ckpt_list),$ckpt_obj"
         }

         set_queue $queue new_val
      }
   } else {
      set q_list ""
      foreach elem $queue_list {
         if [ string compare $q_list "" ] {
            set q_list "$q_list,$elem"
         } else {
            set q_list "$elem"
         }
      }
      set my_change(queue_list) $q_list
      set_checkpointobj $ckpt_obj my_change
   }
}

#****** sge_procedures/get_pe_ckpt_version() ***********************************
#  NAME
#     get_pe_ckpt_version() -- get information about used qconf version
#
#  SYNOPSIS
#     get_pe_ckpt_version { } 
#
#  FUNCTION
#     This procedure returns 0 for qconf supporting queue_list in pe and ckpt
#     objects, otherwise 1.
#
#  INPUTS
#
#  RESULT
#     0 - qconf supporting queue_list in pe and ckpt
#     1 - queue has pe_list and ckpt_list reference pointer 
#
#*******************************************************************************
proc get_pe_ckpt_version {} {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT pe_for_version_check_result

   if { [string compare "undefined" $pe_for_version_check_result] != 0} {
      return $pe_for_version_check_result
   }

   set version 0

   set change(pe_name) "pe_for_version_check"

   catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sp pe_for_version_check" } result
   if { [string match "*xuser_lists*" $result] != 1 } {
      add_pe change 0  ;# no version check (infinitive loop!)
   }
   catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sp pe_for_version_check" } result
   if { [string match "*queue_list*" $result] } {
      set version 0
   } else {
      set version 1
   }
   set pe_for_version_check_result $version
   catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dp pe_for_version_check" } result
   puts $CHECK_OUTPUT $result
   return $version
}

#                                                             max. column:     |
#
#****** sge_procedures/resolve_version() ******
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
#****** sge_procedures/get_qmaster_spool_dir() ******
# 
#  NAME
#     get_qmaster_spool_dir() -- return path to qmaster spool directory
#
#  SYNOPSIS
#     get_qmaster_spool_dir { } 
#
#  FUNCTION
#     This procedure returns the actual qmaster spool directory 
#     (or "" in case of an error)
#
#  RESULT
#     string with actual spool directory of qmaster
#
#
#  SEE ALSO
#     sge_procedures/get_execd_spool_dir()
#*******************************
proc get_qmaster_spool_dir {} {
  get_config global_config
  if { [info exist global_config(qmaster_spool_dir) ] != 0 } {
     return $global_config(qmaster_spool_dir)
  } else {
     return "unknown"
  }
}

#                                                             max. column:     |
#****** sge_procedures/get_execd_spool_dir() ******
# 
#  NAME
#     get_execd_spool_dir() -- return spool dir for exec host
#
#  SYNOPSIS
#     get_execd_spool_dir { host } 
#
#  FUNCTION
#     This procedure returns the actual execd spool directory on the given host.
#     If no local spool directory is specified for this host, the global 
#     configuration is used. If an error accurs the procedure returns "".
#
#  INPUTS
#     host - host name with execd installed on
#
#  RESULT
#     string
#
#  SEE ALSO
#     sge_procedures/get_qmaster_spool_dir()
#*******************************
proc get_execd_spool_dir {host} {
  global CHECK_OUTPUT 
  get_config host_config $host
  if { [info exist host_config(execd_spool_dir) ] == 0 } {
     debug_puts "--> no special execd_spool_dir for host $host"
     get_config host_config
  }
  if { [info exist host_config(execd_spool_dir) ] != 0 } {
     return $host_config(execd_spool_dir)
  } else {
     return "unknown"
  }
}


#****** sge_procedures/check_messages_files() **********************************
#  NAME
#     check_messages_files() -- check messages files for errors and warnings
#
#  SYNOPSIS
#     check_messages_files { } 
#
#  FUNCTION
#     This procedure reads in all cluster messages files from qmaster and
#     execd and returns the messages with errors or warnings.
#
#  RESULT
#     string with parsed file output
#
#*******************************************************************************
proc check_messages_files { } {
   global CHECK_OUTPUT CHECK_CORE_EXECD CHECK_CORE_MASTER


   set full_info ""

   foreach host $CHECK_CORE_EXECD {
      set status [ check_execd_messages $host 1] 
      append full_info "\n=========================================\n"
      append full_info "execd: $host\n"
      append full_info "file : [check_execd_messages $host 2]\n"
      append full_info "=========================================\n"
      append full_info $status
   }

   set status [ check_schedd_messages 1] 
   append full_info "\n=========================================\n"
   append full_info "schedd: $CHECK_CORE_MASTER\n"
   append full_info "file   : [check_schedd_messages 2]\n"
   append full_info "=========================================\n"
   append full_info $status

   set status [ check_qmaster_messages 1] 
   append full_info "\n=========================================\n"
   append full_info "qmaster: $CHECK_CORE_MASTER\n"
   append full_info "file   : [check_qmaster_messages 2]\n"
   append full_info "=========================================\n"
   append full_info $status
   return $full_info
}


#****** sge_procedures/get_qmaster_messages_file() *****************************
#  NAME
#     get_qmaster_messages_file() -- get path to qmaster's messages file
#
#  SYNOPSIS
#     get_qmaster_messages_file { } 
#
#  FUNCTION
#     This procedure returns the path to the running qmaster's messages file
#
#  RESULT
#     path to qmaster's messages file
#
#  SEE ALSO
#     sge_procedures/get_execd_messages_file()
#     sge_procedures/get_schedd_messages_file()
#
#*******************************************************************************
proc get_qmaster_messages_file { } {
   return [ check_qmaster_messages 2 ]
}

#****** sge_procedures/check_qmaster_messages() ********************************
#  NAME
#     check_qmaster_messages() -- get qmaster messages file content
#
#  SYNOPSIS
#     check_qmaster_messages { { show_mode 0 } } 
#
#  FUNCTION
#     This procedure locates the qmaster messages file (using qconf -sconf) 
#     and returns the output of cat.
#
#  INPUTS
#     { show_mode 0 } - if not 0: return only warning and error lines
#                       if     2: return only path to qmaster messages file
#
#  RESULT
#     output string
#
#  SEE ALSO
#     sge_procedures/check_execd_messages()
#     sge_procedures/check_schedd_messages()
#*******************************************************************************
proc check_qmaster_messages { { show_mode 0 } } {
   global CHECK_ARCH CHECK_PRODUCT_ROOT CHECK_HOST CHECK_USER
   global CHECK_OUTPUT CHECK_CORE_MASTER

   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"
   set program_arg "-sconf global" 
   set output [ start_remote_prog $CHECK_HOST $CHECK_USER $program $program_arg]

   set output [ split $output "\n" ]
   set spool_dir "unkown"
   foreach line $output {
      if { [ string first "qmaster_spool_dir" $line ] >= 0 } {
         set spool_dir [ lindex $line 1 ]
      }
   }
   puts $CHECK_OUTPUT "\"$spool_dir\""

   set messages_file "$spool_dir/messages"

   if { $show_mode == 2 } {
      return $messages_file
   }   

   set return_value ""

   get_file_content $CHECK_CORE_MASTER $CHECK_USER $messages_file
   for { set i 1 } { $i <= $file_array(0) } { incr i 1 } {
       set line $file_array($i)
       if { ( [ string first "|E|" $line ] >= 0 )   || 
            ( [ string first "|W|" $line ] >= 0 )   ||
            ( $show_mode == 0               )  }   {
            append return_value "line $i: $line\n"
       }
   }
   return $return_value
}

#****** sge_procedures/get_schedd_messages_file() ******************************
#  NAME
#     get_schedd_messages_file() -- get path to scheduler's messages file
#
#  SYNOPSIS
#     get_schedd_messages_file { } 
#
#  FUNCTION
#     This procedure returns the path to the running scheduler's messages file
#
#  RESULT
#     path to scheduler's messages file
#
#  SEE ALSO
#     sge_procedures/get_execd_messages_file()
#     sge_procedures/get_qmaster_messages_file()
#*******************************************************************************
proc get_schedd_messages_file { } {
   return [ check_schedd_messages 2 ]
}

#****** sge_procedures/check_schedd_messages() *********************************
#  NAME
#     check_schedd_messages() -- get schedulers messages file content
#
#  SYNOPSIS
#     check_schedd_messages { { show_mode 0 } } 
#
#  FUNCTION
#     This procedure locates the schedd messages file (using qconf -sconf) 
#     and returns the output of cat.
#
#  INPUTS
#     { show_mode 0 } - if not 0: return only warning and error lines
#                       if     2: return only path to schedd messages file
#
#  RESULT
#     output string
#
#  SEE ALSO
#     sge_procedures/check_execd_messages()
#     sge_procedures/check_qmaster_messages()
#*******************************************************************************
proc check_schedd_messages { { show_mode 0 } } {
   global CHECK_ARCH CHECK_PRODUCT_ROOT CHECK_HOST CHECK_USER
   global CHECK_OUTPUT CHECK_CORE_MASTER

   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"
   set program_arg "-sconf global" 
   set output [ start_remote_prog $CHECK_HOST $CHECK_USER $program $program_arg]

   set output [ split $output "\n" ]
   set spool_dir "unkown"
   foreach line $output {
      if { [ string first "qmaster_spool_dir" $line ] >= 0 } {
         set spool_dir [ lindex $line 1 ]
      }
   }

   set messages_file "$spool_dir/schedd/messages"

   if { $show_mode == 2 } {
      return $messages_file
   }   

   set return_value ""

   get_file_content $CHECK_CORE_MASTER $CHECK_USER $messages_file
   for { set i 1 } { $i <= $file_array(0) } { incr i 1 } {
       set line $file_array($i)
       if { ( [ string first "|E|" $line ] >= 0 )   || 
            ( [ string first "|W|" $line ] >= 0 )   ||
            ( $show_mode == 0               )  }   {
            append return_value "line $i: $line\n"
       }
   }
   return $return_value
}


#****** sge_procedures/get_execd_messages_file() *******************************
#  NAME
#     get_execd_messages_file() -- get messages file path of execd
#
#  SYNOPSIS
#     get_execd_messages_file { hostname } 
#
#  FUNCTION
#     This procedure returns the full path to the given execd's messages file
#
#  INPUTS
#     hostname - hostname where the execd is running
#
#  RESULT
#     path to messages file of the given execd host
#
#  SEE ALSO
#     sge_procedures/get_qmaster_messages_file()
#     sge_procedures/get_schedd_messages_file()
#
#*******************************************************************************
proc get_execd_messages_file { hostname } {
   return [ check_execd_messages $hostname 2 ]
}

#****** sge_procedures/check_execd_messages() **********************************
#  NAME
#     check_execd_messages() -- get execd messages file content
#
#  SYNOPSIS
#     check_execd_messages { hostname { show_mode 0 } } 
#
#  FUNCTION
#     This procedure locates the execd messages file (using qconf -sconf) 
#     and returns the output of cat.
#
#  INPUTS
#     hostname        - hostname of execd
#     { show_mode 0 } - if not 0: return only warning and error lines
#                       if     2: return only path to execd messages file
#
#  RESULT
#     output string
#
#  SEE ALSO
#     sge_procedures/check_qmaster_messages()
#     sge_procedures/check_schedd_messages()
#*******************************************************************************
proc check_execd_messages { hostname { show_mode 0 } } {
   global CHECK_ARCH CHECK_PRODUCT_ROOT CHECK_HOST CHECK_USER
   global CHECK_OUTPUT

   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"
   set program_arg "-sconf $hostname" 
   set output [ start_remote_prog $CHECK_HOST $CHECK_USER $program $program_arg]
   if { [string first "execd_spool_dir" $output ] < 0 } {
      set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"
      set program_arg "-sconf global" 
      set output [ start_remote_prog $CHECK_HOST $CHECK_USER $program $program_arg]
   }

   set output [ split $output "\n" ]
   set spool_dir "unkown"
   foreach line $output {
      if { [ string first "execd_spool_dir" $line ] >= 0 } {
         set spool_dir [ lindex $line 1 ]
      }
   }
   puts $CHECK_OUTPUT "\"$spool_dir\""

   set messages_file "$spool_dir/$hostname/messages"
   if { $show_mode == 2 } {
      return $messages_file
   }

   set return_value ""

   get_file_content $hostname $CHECK_USER $messages_file
   for { set i 1 } { $i <= $file_array(0) } { incr i 1 } {
       set line $file_array($i)
       if { ( [ string first "|E|" $line ] >= 0 ) || 
            ( [ string first "|W|" $line ] >= 0 ) ||
            ( $show_mode == 0 )                }  {
            append return_value "line $i: $line\n"
       }
   }
   return $return_value
} 


#                                                             max. column:     |
#****** sge_procedures/get_exechost() ******
# 
#  NAME
#     get_exechost -- get exec host configuration
#
#  SYNOPSIS
#     get_exechost { change_array host } 
#
#  FUNCTION
#     Get the exec host specific configuration settings. The given variable
#     is used to save the configuration settings.
#
#  INPUTS
#     change_array - name of an array variable that will get set by get_exechost
#     host         - name of an execution host
#
#  RESULT
#     The array is build like follows:
#
#     set change_array(user_list)   "deadlineusers"
#     set change_array(load_scaling) "NONE"
#     ....
#
#
#     Here the possible change_array values with some typical settings:
#
#     hostname                   myhost.mydomain
#     load_scaling               NONE
#     complex_list               test
#     complex_values             NONE
#     user_lists                 deadlineusers
#     xuser_lists                NONE
#     projects                   NONE
#     xprojects                  NONE
#     usage_scaling              NONE
#     resource_capability_factor 0.000000       
# 
#  EXAMPLE
#     get_exechost change_array expo1
#     puts $change_array(user_list)
#
#
#  SEE ALSO
#     sge_procedures/set_exechost()
#*******************************
proc get_exechost {change_array host} {
  global CHECK_PRODUCT_ROOT CHECK_ARCH
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-se" "$host" } result ]
  if { $catch_result == 0 } {
     # split each line as listelement
     set help [split $result "\n"]

     foreach elem $help {
        set id [lindex $elem 0]
        set value [lrange $elem 1 end]
        set chgar($id) $value
     }
  } else {
    add_proc_error "get_exechost" "-1" "qconf binary not found"
  }
}


#****** sge_procedures/submit_error_job() **************************************
#  NAME
#     submit_error_job() -- submit job which will get error state
#
#  SYNOPSIS
#     submit_error_job { jobargs } 
#
#  FUNCTION
#     This procedure is submitting a job with a wrong shell option (-S). This
#     will set the job in error state. (E)
#
#  INPUTS
#     jobargs - job arguments (e.g. -o ... -e ... jobscript path)
#
#  RESULT
#     job id 
#
#  SEE ALSO
#     sge_procedures/submit_error_job()
#     sge_procedures/submit_waitjob_job()
#     sge_procedures/submit_time_job()
#*******************************************************************************
proc submit_error_job { jobargs } {
    return [submit_job "-S __no_shell $jobargs"]
}

#****** sge_procedures/submit_time_job() ***************************************
#  NAME
#     submit_time_job() -- Submit a job with execution time
#
#  SYNOPSIS
#     submit_time_job { jobargs } 
#
#  FUNCTION
#     This procedure will submit a job with the -a option. The start time
#     is set to function call time + 2 min.
#
#  INPUTS
#     jobargs - job arguments (e.g. -o ... -e ... job start script path)
#
#  RESULT
#     job id
#
#  SEE ALSO
#     sge_procedures/submit_error_job()
#     sge_procedures/submit_waitjob_job()
#     sge_procedures/submit_time_job()
#*******************************************************************************
proc submit_time_job { jobargs } {

   set hour   [exec date "+%H"]
   set minute [exec date "+%M"]

   if { [string first "0" $hour] == 0 } {
      set hour [string index $hour 1 ]
   }  
   if { [string first "0" $minute] == 0 } {
      set minute [string index $minute 1 ]
   }  
  
   if {$minute < 58 } {
     set minute [expr ($minute + 2) ]
   } else {
     set minute [expr ($minute + 2 - 60) ]
      if {$hour < 23 } {
         set hour [expr ($hour + 1) ]
      } else {
         set hour "00"
      }
   }

   set rhour $hour
   set rminute $minute

   if {$hour < 10} {
     set rhour "0$hour"
   }
   if {$minute < 10} {
     set rminute "0$minute"
   }

   set start "[exec date +\%Y\%m\%d]$rhour$rminute"
   set result [submit_job "-a $start $jobargs"] 
   return $result
}


#****** sge_procedures/submit_waitjob_job() ************************************
#  NAME
#     submit_waitjob_job() -- submit job with hold_jid (wait for other job)
#
#  SYNOPSIS
#     submit_waitjob_job { jobargs wait_job_id } 
#
#  FUNCTION
#     This procedure will submit a job with hold_jid option set. This means that
#     the job is not started while an other job is running
#
#  INPUTS
#     jobargs     - additional job arguments ( jobscript, -e -o option ...)
#     wait_job_id - job id to wait for
#
#  RESULT
#     job id of hold_jid job
#
#  SEE ALSO
#     sge_procedures/submit_error_job()
#     sge_procedures/submit_waitjob_job()
#     sge_procedures/submit_time_job()
#*******************************************************************************
proc submit_waitjob_job { jobargs wait_job_id} {
   return [submit_job "-hold_jid $wait_job_id $jobargs"]
}



#                                                             max. column:     |
#****** sge_procedures/set_exechost() ******
# 
#  NAME
#     set_exechost -- set/change exec host configuration
#
#  SYNOPSIS
#     set_exechost { change_array host } 
#
#  FUNCTION
#     Set the exec host configuration corresponding to the content of the 
#     change_array.
#
#  INPUTS
#     change_array - name of an array variable that will be set by set_exechost
#     host         - name of an execution host
#
#  RESULT
#     The array should look like follows:
#
#     set change_array(user_list)   "deadlineusers"
#     set change_array(load_scaling) "NONE"
#     ....
#     (every value that is set will be changed)
#
#
#     Here the possible change_array values with some typical settings:
#
#     hostname                   myhost.mydomain
#     load_scaling               NONE
#     complex_list               test
#     complex_values             NONE
#     user_lists                 deadlineusers
#     xuser_lists                NONE
#     projects                   NONE
#     xprojects                  NONE
#     usage_scaling              NONE
#     resource_capability_factor 0.000000       
# 
#     return value:
#     -100 :	unknown error
#     -1   :	on timeout
#        0 :	ok
#
#  EXAMPLE
#     get_exechost myconfig expo1
#     set myconfig(user_lists) NONE
#     set_exechost myconfig expo1
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/get_exechost()
#*******************************
proc set_exechost { change_array host } {
# the array should look like this:
#
# set change_array(load_scaling) NONE
# ....
# (every value that is set will be changed)
# hostname                   myhostname
# load_scaling               NONE
# complex_list               test
# complex_values             NONE
# user_lists                 deadlineusers
# xuser_lists                NONE
# projects                   NONE
# xprojects                  NONE
# usage_scaling              NONE
# resource_capability_factor 0.000000       
#
# returns 
# -1   on timeout
# 0    if ok

  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_HOST

  upvar $change_array chgar

  set values [array names chgar]

  get_exechost old_values $host

  set vi_commands ""
  foreach elem $values {
     # continue on unchangeable values
     if { [string compare $elem "load_values"] == 0 } {
        continue;
     } 
     if { [string compare $elem "processors"] == 0 } {
        continue;
     } 
     if { [string compare $elem "reschedule_unknown_list"] == 0 } {
        continue;
     } 

     # this will quote any / to \/  (for vi - search and replace)
     set newVal $chgar($elem)
   
     if {[info exists old_values($elem)]} {
        if { $newVal == "" } {
           lappend vi_commands ":%s/^$elem .*$//\n"
        } else {
           set newVal1 [split $newVal {/}]
           set newVal [join $newVal1 {\/}]
           lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
        }
     } else {
        lappend vi_commands "A\n$elem  $newVal"
        lappend vi_commands [format "%c" 27]
     }
  } 
  set CHANGED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S] "*" ]
  set result [handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-me $host" $vi_commands "modified" $CHANGED]
  if { $result == -2 } {
     set result 0
  }
  if { $result != 0 } {
     add_proc_error "set_exechost" -1 "could not modifiy exechost $host"
     set result -1
  }
  return $result
}




#****** sge_procedures/get_loadsensor_path() ***********************************
#  NAME
#     get_loadsensor_path() -- get loadsensor for host
#
#  SYNOPSIS
#     get_loadsensor_path { host } 
#
#  FUNCTION
#     This procedure will read the load sensor path for the given host
#
#  INPUTS
#     host - hostname to get loadsensor for
#
#  RESULT
#     full path name of loadsensor 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc get_loadsensor_path { host } {
   global CHECK_OUTPUT
   global ts_host_config

   if { [ info exists ts_host_config($host,loadsensor) ] != 1 } {
      add_proc_error "get_loadsensor_path" -1 "no host configuration found for host \"$host\""
      return ""
   }

   set loadsensor $ts_host_config($host,loadsensor)
   return $loadsensor
}

#
#                                                             max. column:     |
#
#****** sge_procedures/get_gid_range() ******
#  NAME
#     get_gid_range() -- get gid range for user   
#
#  SYNOPSIS
#     get_gid_range { user port } 
#
#  FUNCTION
#     This procedure ist used in the install_core_system test. It returns the
#     gid range of the requested user and port
#
#  INPUTS
#     user - user name
#     port - port number on which the cluster commd is running
#
#  RESULT
#     gid range, e.g. 13501-13700
#
#  SEE ALSO
#     ???/???
#*******************************
#
proc get_gid_range { user port } {

  global CHECK_OUTPUT
  global ts_user_config

  if { [ info exists ts_user_config($port,$user) ] } {
     return $ts_user_config($port,$user)
  }
  add_proc_error "get_gid_range" -1 "no gid range defined for user $user on port $port"
  return ""
}


#                                                             max. column:     |
#****** sge_procedures/move_qmaster_spool_dir() ******
# 
#  NAME
#     move_qmaster_spool_dir -- ??? 
#
#  SYNOPSIS
#     move_qmaster_spool_dir { new_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     new_spool_dir - ??? 
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
proc move_qmaster_spool_dir { new_spool_dir } {
  global CHECK_CORE_MASTER CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_ROOT
  global CHECK_HOST

  set old_spool_dir [ get_qmaster_spool_dir ]
 
  
  if { [ string compare $old_spool_dir $new_spool_dir ] == 0 } {
     add_proc_error "move_qmaster_spool_dir" -1 "old and new spool dir are the same"
     return
  }
 
  if { [ string compare "unknown" $old_spool_dir] == 0 } { 
     add_proc_error "move_qmaster_spool_dir" -1 "can't get qmaster spool dir"
     return
  }

  if { [string length $new_spool_dir] <= 10  } {
     # just more security (do not create undefined dirs or something like that)
     add_proc_error "move_qmaster_spool_dir" -1 "please use path with size > 10 characters"
     return
  } 

  # change qmaster spool dir and shutdown the qmaster and scheduler
#  this was the old way before 5.3.beta2. Now we can't change qmaster_spool_dir in
#  a running system. We have to do it manually.
#  set change_array(qmaster_spool_dir) $new_spool_dir
#  set_config change_array "global"
  shutdown_master_and_scheduler $CHECK_CORE_MASTER $old_spool_dir

  set vi_commands ""
 #  ":%s/^$elem .*$/$elem  $newVal/\n"
  set newVal1 [split $new_spool_dir {/}]
  set newVal [join $newVal1 {\/}]

  lappend vi_commands ":%s/^qmaster_spool_dir .*$/qmaster_spool_dir    $newVal/\n"
  set vi_binary [get_binary_path $CHECK_HOST "vim"]
  set result [ handle_vi_edit "$vi_binary" "$CHECK_PRODUCT_ROOT/default/common/configuration" "$vi_commands" "" ] 
  puts $CHECK_OUTPUT "result: \"$result\""
  if { $result != 0 } {
     add_proc_error "shadowd_kill_master_and_scheduler" -1 "edit error when changing global configuration"
  } 

  puts $CHECK_OUTPUT "make copy of spool directory ..."
  # now copy the entries  
  set result [ start_remote_tcl_prog $CHECK_CORE_MASTER $CHECK_USER "file_procedures.tcl" "copy_directory" "$old_spool_dir $new_spool_dir" ]
  if { [ string first "no errors" $result ] < 0 } {
      add_proc_error "shadowd_kill_master_and_scheduler" -1 "error moving qmaster spool dir"
  }

  puts $CHECK_OUTPUT "starting up qmaster ..."
  startup_qmaster
  wait_for_load_from_all_queues 300

  set changed_spool_dir [ get_qmaster_spool_dir ]
  if { [string compare $changed_spool_dir $new_spool_dir] != 0 } {
     add_proc_error "shadowd_kill_master_and_scheduler" -1 "error changing qmaster spool dir"
  }

}

#                                                             max. column:     |
#****** sge_procedures/reset_schedd_config() ******
# 
#  NAME
#     reset_schedd_config -- set schedd configuration default values
#
#  SYNOPSIS
#     reset_schedd_config { } 
#
#  FUNCTION
#     This procedure will call set_schedd_config with default values 
#
#  RESULT
#       -1 : timeout error
#        0 : ok
#
#
#  NOTES
#     The default values are:
#     
#     SGE system:
#    
#     algorithm                   "default"
#     schedule_interval           "0:0:15"
#     maxujobs                    "0"
#     queue_sort_method           "load"
#     user_sort                   "false"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     SGEEE differences:
#     queue_sort_method           "share"
#     user_sort                   "false"
#     sgeee_schedule_interval     "00:01:00"
#     halftime                    "168"
#     usage_weight_list           "cpu=1,mem=0,io=0"
#     compensation_factor         "5"
#     weight_user                 "0.2"
#     weight_project              "0.2"
#     weight_jobclass             "0.2"
#     weight_department           "0.2"
#     weight_job                  "0.2"
#     weight_tickets_functional   "0"
#     weight_tickets_share        "0"
#     weight_tickets_deadline     "10000"
#
#  SEE ALSO
#     sge_procedures/set_schedd_config()
#*******************************
proc reset_schedd_config {} {
  global CHECK_PRODUCT_TYPE
 
  set default_array(algorithm)                  "default"
  set default_array(schedule_interval)          "0:0:10"
  set default_array(maxujobs)                   "0"
  set default_array(queue_sort_method)          "load"
  set default_array(user_sort)                  "false"
  set default_array(job_load_adjustments)       "np_load_avg=0.15"
  set default_array(load_adjustment_decay_time) "0:7:30"
  set default_array(load_formula)               "np_load_avg"
  set default_array(schedd_job_info)            "true"

# this is sgeee
  if { [string compare $CHECK_PRODUCT_TYPE "sgeee"] == 0 } {
     set default_array(queue_sort_method)          "share"
     set default_array(sgeee_schedule_interval)    "00:00:40"
     set default_array(halftime)                   "168"
     set default_array(usage_weight_list)          "cpu=1,mem=0,io=0"
     set default_array(compensation_factor)        "5"
     set default_array(weight_user)                "0.2"
     set default_array(weight_project)             "0.2"
     set default_array(weight_jobclass)            "0.2"
     set default_array(weight_department)          "0.2"
     set default_array(weight_job)                 "0.2"
     set default_array(weight_tickets_functional)  "0"
     set default_array(weight_tickets_share)       "0"
     set default_array(weight_tickets_deadline)    "10000"
  }

  set ret_value [ set_schedd_config default_array ]

  if { $ret_value != 0 } {
     add_proc_error "reset_schedd_config" $ret_value "error set_schedd_config - call"
  } 

  return $ret_value
}


#                                                             max. column:     |
#****** sge_procedures/get_hosts() ******
# 
#  NAME
#     get_hosts -- ??? 
#
#  SYNOPSIS
#     get_hosts { } 
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
proc get_hosts { } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT

  set host_list ""
  set catch_result [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sh" } result ]
  if { $catch_result != 0 } {
     add_proc_error "get_hosts" "-1" "qconf error or binary not found ($CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf)\n$result"
     return ""
  } 

  foreach elem $result {
     lappend host_list $elem
  }  

  foreach elem $host_list {
     puts $CHECK_OUTPUT "\"$elem\""
  }
  return $host_list

}

#                                                             max. column:     |
#****** sge_procedures/get_config() ******
# 
#  NAME
#     get_config -- get global or host configuration settings
#
#  SYNOPSIS
#     get_config { change_array {host "global"} } 
#
#  FUNCTION
#     Get the global or host specific configuration settings.
#
#  INPUTS
#     change_array    - name of an array variable that will get set by 
#                       get_config
#     {host "global"} - get configuration for a specific hostname (host) 
#                       or get the global configuration (global)
#
#  RESULT
#     The change_array variable is build as follows:
#
#     set change_array(xterm)   "/bin/xterm"
#     set change_array(enforce_project) "true"
#     ...
#     
#
#  EXAMPLE
#     get_config gcluster1 lobal
#     puts $cluster1(qmaster_spool_dir)
#     
#     Here the possible change_array values with some typical settings:
#     
#     qmaster_spool_dir    /../default/spool/qmaster
#     execd_spool_dir      /../default/spool
#     qsi_common_dir       /../default/common/qsi
#     binary_path          /../bin
#     mailer               /usr/sbin/Mail
#     xterm                /usr/bin/X11/xterm
#     load_sensor          none
#     prolog               none
#     epilog               none
#     shell_start_mode     posix_compliant
#     login_shells         sh,ksh,csh,tcsh
#     min_uid              0
#     min_gid              0
#     user_lists           none
#     xuser_lists          none
#     projects             none
#     xprojects            none
#     load_report_time     00:01:00
#     stat_log_time        12:00:00
#     max_unheard          00:02:30
#     loglevel             log_info
#     enforce_project      false
#     administrator_mail   none
#     set_token_cmd        none
#     pag_cmd              none
#     token_extend_time    none
#     shepherd_cmd         none
#     qmaster_params       none
#     schedd_params        none
#     execd_params         none
#     finished_jobs        0
#     gid_range            13001-13100
#     admin_user           crei
#     qlogin_command       telnet
#     qlogin_daemon        /usr/etc/telnetd
#     
#
#  SEE ALSO
#     sge_procedures/set_config()
#*******************************
proc get_config { change_array {host global}} {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sconf" "$host"} result ]
  if { $catch_result != 0 } {
     add_proc_error "get_config" "-1" "qconf error or binary not found ($CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf)\n$result"
     return
  } 

  # split each line as listelement
  set help [split $result "\n"]
  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     if { [string compare $value ""] != 0 } {
       set chgar($id) $value
     }
  }
}

#****** sge_procedures/get_checkpointobj() *************************************
#  NAME
#     get_checkpointobj() -- get checkpoint configuration information
#
#  SYNOPSIS
#     get_checkpointobj { ckpt_obj change_array } 
#
#  FUNCTION
#     Get the actual configuration settings for the named checkpoint object
#
#  INPUTS
#     ckpt_obj     - name of the checkpoint object
#     change_array - name of an array variable that will get set by 
#                    get_checkpointobj
#
#  SEE ALSO
#     sge_procedures/set_checkpointobj()
#     sge_procedures/get_queue() 
#     sge_procedures/set_queue()
#*******************************************************************************
proc get_checkpointobj { ckpt_obj change_array } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sckpt" "$ckpt_obj"} result ]
  if { $catch_result != 0 } {
     add_proc_error "get_checkpointobj" "-1" "qconf error or binary not found ($CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf)\n$result"
     return
  } 

  # split each line as listelement
  set help [split $result "\n"]
  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     if { [string compare $value ""] != 0 } {
       set chgar($id) $value
     }
  }
}

#****** sge_procedures/get_complex() *******************************************
#  NAME
#     get_complex() -- get complex configuration settings
#
#  SYNOPSIS
#     get_complex { change_array complex_list } 
#
#  FUNCTION
#     Get the complex configuration for a specific complex list
#
#  INPUTS
#     change_array - name of an array variable that will get set by 
#                    get_config
#     complex_list - name of a complex list
#
#  RESULT
#     The change_array variable is build as follows:
#
#                      
#     set change_array(mem_free) "mf MEMORY 0 <= YES NO 0"   
#   
#     index (mem_free) is the complex name
#     value is "shortcut type value relop requestable consumable default"
#    
#*******************************************************************************
proc get_complex { change_array complex_list } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sc" "$complex_list"} result ]
  if { $catch_result != 0 } {
     add_proc_error "get_complex" "-1" "qconf error or binary not found ($CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf)\n$result"
     return
  } 

  # split each line as listelement
  set help [split $result "\n"]
  foreach elem $help {
     set id [lindex $elem 0]
     if { [ string first "#" $id ]  != 0 } {
        set value [lrange $elem 1 end]
        if { [string compare $value ""] != 0 } {
           set chgar($id) $value
        }
     }
  }
}



#                                                             max. column:     |
#****** sge_procedures/set_config() ******
# 
#  NAME
#     set_config -- change global or host specific configuration
#
#  SYNOPSIS
#     set_config { change_array {host global}{do_add 0} } 
#
#  FUNCTION
#     Set the cluster global or exec host local configuration corresponding to 
#     the content of the change_array.
#
#  INPUTS
#     change_array  - name of an array variable that will be set by get_config
#     {host global} - set configuration for a specific hostname (host) or set
#                     the global configuration (global)
#     {do_add 0}    - if 1: this is a new configuration, no old one exists)
#
#  RESULT
#     -1 : timeout
#      0 : ok
#
#     The change_array variable is build as follows:
#
#     set change_array(xterm)   "/bin/xterm"
#     set change_array(enforce_project) "true"
#     ...
#     (every value that is set will be changed)
#
#
#  EXAMPLE
#     get_config gcluster1 lobal
#     set cluster1(qmaster_spool_dir) "/bla/bla/tmp"
#     set_config cluster1
#     
#     Here the possible change_array values with some typical settings:
#     
#     qmaster_spool_dir    /../default/spool/qmaster
#     execd_spool_dir      /../default/spool
#     qsi_common_dir       /../default/common/qsi
#     binary_path          /../bin
#     mailer               /usr/sbin/Mail
#     xterm                /usr/bin/X11/xterm
#     load_sensor          none
#     prolog               none
#     epilog               none
#     shell_start_mode     posix_compliant
#     login_shells         sh,ksh,csh,tcsh
#     min_uid              0
#     min_gid              0
#     user_lists           none
#     xuser_lists          none
#     projects             none
#     xprojects            none
#     load_report_time     00:01:00
#     stat_log_time        12:00:00
#     max_unheard          00:02:30
#     loglevel             log_info
#     enforce_project      false
#     administrator_mail   none
#     set_token_cmd        none
#     pag_cmd              none
#     token_extend_time    none
#     shepherd_cmd         none
#     qmaster_params       none
#     schedd_params        none
#     execd_params         none
#     finished_jobs        0
#     gid_range            13001-13100
#     admin_user           crei
#     qlogin_command       telnet
#     qlogin_daemon        /usr/etc/telnetd
#
#  SEE ALSO
#     sge_procedures/get_config()
#*******************************
proc set_config { change_array {host global} {do_add 0} {ignore_error 0}} {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_USER
  upvar $change_array chgar
  set values [array names chgar]

  if { $do_add == 0 } {
     get_config old_values $host
  }

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal $chgar($elem)
   
     if {[info exists old_values($elem)]} {
        if { $newVal == "" } {
          lappend vi_commands ":%s/^$elem .*$//\n"
        } else {
          set newVal1 [split $newVal {/}]
          set newVal [join $newVal1 {\/}]
          lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
        }
     } else {
        lappend vi_commands "A\n$elem  $newVal"
        lappend vi_commands [format "%c" 27]
     }
  } 
  set GIDRANGE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_CONFIG_CONF_GIDRANGELESSTHANNOTALLOWED_I] "*"]

  if { [resolve_version] > 2 } {
     set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
     set ADDED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  } else {
     set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_CONFIG_MODIFIEDINLIST_SSS] $CHECK_USER "*" "*"]
     set ADDED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_CONFIG_ADDEDTOLIST_SSS] $CHECK_USER "*" "*"]
  }


  set EDIT_FAILED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PARSE_EDITFAILED]]

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-mconf $host" $vi_commands $MODIFIED $EDIT_FAILED $ADDED $GIDRANGE ]
  
  if { ($ignore_error == 1) && ($result == -4) } {
     # ignore error -4 
  } else {
    if { ($result != 0) && ($result != -3) } {
      add_proc_error "set_config" -1 "could not add or modify configruation for host $host ($result)"
    }
  }
  return $result
}

#****** sge_procedures/set_complex() *******************************************
#  NAME
#     set_complex() -- change complex configuration for a complex list
#
#  SYNOPSIS
#     set_complex { change_array complex_list } 
#
#  FUNCTION
#     Set the complex configuration for a specific complex list
#
#  INPUTS
#     change_array - name of an array variable that contains the new complex 
#                    definition
#     complex_list - name of the complex list to change
#
#  RESULT
#     The change_array variable is build as follows:
#
#     set change_array(mem_free) "mf MEMORY 0 <= YES NO 0"   
#   
#     index (mem_free) is the complex name
#     value is "shortcut type value relop requestable consumable default"
#
#  EXAMPLE
#     set mycomplex(load_long) "ll DOUBLE 55.55 >= NO NO 0"
#     set_complex mycomplex host
#
#*******************************************************************************
proc set_complex { change_array complex_list { create 0 } } {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer
  global CHECK_CORE_MASTER
  upvar $change_array chgar
  set values [array names chgar]

  if { $create == 0 } {
     get_complex old_values $complex_list
  }

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal $chgar($elem)
     if {[info exists old_values($elem)]} {
        if { $newVal == "" } {
          lappend vi_commands ":%s/^$elem .*$//\n"
        } else {
          set newVal1 [split $newVal {/}]
          set newVal [join $newVal1 {\/}]
          lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
        }
     } else {
        lappend vi_commands "A\n$elem  $newVal"
        lappend vi_commands [format "%c" 27]
     }
  } 
  set EDIT_FAILED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PARSE_EDITFAILED]]
  set MODIFIED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_MULTIPLY_MODIFIEDIN]]
  set ADDED       [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_MULTIPLY_ADDEDTO]]


  if { $create == 0 } {
     set result [ handle_vi_edit "echo" "\"\"\nSGE_ENABLE_MSG_ID=1\nexport SGE_ENABLE_MSG_ID\n$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -mc $complex_list" $vi_commands $MODIFIED $EDIT_FAILED $ADDED ]
  } else {
     set result [ handle_vi_edit "echo" "\"\"\nSGE_ENABLE_MSG_ID=1\nexport SGE_ENABLE_MSG_ID\n$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -ac $complex_list" $vi_commands $ADDED $EDIT_FAILED $MODIFIED ]
  }
  if { $result != 0  } {
     add_proc_error "set_complex" -1 "could not modify complex $complex_list ($result)"
  }
  return $result
}



#                                                             max. column:     |
#****** sge_procedures/set_schedd_config() ******
# 
#  NAME
#     set_schedd_config -- change scheduler configuration
#
#  SYNOPSIS
#     set_schedd_config { change_array } 
#
#  FUNCTION
#     Set the scheduler configuration corresponding to the content of the 
#     change_array.
#
#  INPUTS
#     change_array - name of an array variable that will be set by 
#                    set_schedd_config
#  RESULT
#     -1 : timeout
#      0 : ok
#
#  EXAMPLE
#     get_schedd_config myconfig
#     set myconfig(schedule_interval) "0:0:10"
#     set_schedd_config myconfig
#
#  NOTES
#     The array should be build like follows:
#   
#     set change_array(algorithm) default
#     set change_array(schedule_interval) 0:0:15
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#     
#     algorithm                   "default"
#     schedule_interval           "0:0:15"
#     maxujobs                    "0"
#     queue_sort_method           "share"
#     user_sort                   "false"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     In case of a SGEEE - System:
#     
#     sgeee_schedule_interval       "00:01:00"
#     halftime                    "168"
#     usage_weight_list           "cpu=0.34,mem=0.33,io=0.33"
#     compensation_factor         "5"
#     weight_user                 "0"
#     weight_project              "0"
#     weight_jobclass             "0"
#     weight_department           "0"
#     weight_job                  "0"
#     weight_tickets_functional   "0"
#     weight_tickets_share        "0"
#     weight_tickets_deadline     "10000"
#     
#
#  SEE ALSO
#     sge_procedures/get_schedd_config()
#*******************************
proc set_schedd_config { change_array } {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_OUTPUT CHECK_CORE_MASTER
  upvar $change_array chgar

  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {

     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  }
  set CHANGED_SCHEDD_CONFIG [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION]]
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-msconf" $vi_commands $CHANGED_SCHEDD_CONFIG ]  

  if { $result != 0 } {
     add_proc_error "set_schedd_config" -1 "error changing scheduler configuration"
  }
  
  return $result
}

#                                                             max. column:     |
#****** sge_procedures/get_schedd_config() ******
# 
#  NAME
#     get_schedd_config -- get scheduler configuration 
#
#  SYNOPSIS
#     get_schedd_config { change_array } 
#
#  FUNCTION
#     Get the current scheduler configuration     
#
#  INPUTS
#     change_array - name of an array variable that will get set by 
#                    get_schedd_config
#
#  EXAMPLE
#     get_schedd_config test
#     puts $test(schedule_interval)
#
#  NOTES
# 
#     The array is build like follows:
#   
#     set change_array(algorithm) default
#     set change_array(schedule_interval) 0:0:15
#     ....
#
#     Here the possible change_array values with some typical settings:
#     
#     algorithm                   "default"
#     schedule_interval           "0:0:15"
#     maxujobs                    "0"
#     queue_sort_method           "share"
#     user_sort                   "false"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     In case of a SGEEE - System:
#     
#     sgeee_schedule_interval       "00:01:00"
#     halftime                    "168"
#     usage_weight_list           "cpu=0.34,mem=0.33,io=0.33"
#     compensation_factor         "5"
#     weight_user                 "0"
#     weight_project              "0"
#     weight_jobclass             "0"
#     weight_department           "0"
#     weight_job                  "0"
#     weight_tickets_functional   "0"
#     weight_tickets_share        "0"
#     weight_tickets_deadline     "10000"
#
#  SEE ALSO
#     sge_procedures/set_schedd_config()
#*******************************
proc get_schedd_config { change_array } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH
  upvar $change_array chgar

  set catch_return [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -ssconf" } result ]
  if { $catch_return != 0 } {
     add_proc_error "get_schedd_config" "-1" "qconf error or binary not found"
     return
  }

  # split each line as listelement
  set help [split $result "\n"]

  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     set chgar($id) $value
  }
}



#                                                             max. column:     |
#****** sge_procedures/set_queue() ******
# 
#  NAME
#     set_queue -- set or change queue configuration
#
#  SYNOPSIS
#     set_queue { q_name change_array } 
#
#  FUNCTION
#     Set a queue configuration corresponding to the content of the change_array.
#
#  INPUTS
#     q_name       - name of the queue to configure
#     change_array - name of an array variable that will be set by set_queue
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  EXAMPLE
#     get_queue myqueue.q queue1
#     set queue1(load_thresholds) "np_load_avg=3.75" 
#     set_queue myqueue.q queue1
#
#  NOTES
#     the array should look like this:
#
#     set change_array(qname) MYHOST
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all guilty array names (template queue):
#
#     change_array(qname)                "template"
#     change_array(hostname)             "unknown"
#     change_array(seq_no)               "0"
#     change_array(load_thresholds)      "np_load_avg=1.75"
#     change_array(suspend_thresholds)   "NONE"
#     change_array(nsuspend)             "0"
#     change_array(suspend_interval)     "00:05:00"
#     change_array(priority)             "0"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(notify)               "00:00:60"
#     change_array(owner_list)           "NONE"
#     change_array(user_lists)           "NONE"
#     change_array(xuser_lists)          "NONE"
#     change_array(subordinate_list)     "NONE"
#     change_array(complex_list)         "NONE"
#     change_array(complex_values)       "NONE"
#     change_array(projects)             "NONE"
#     change_array(xprojects)            "NONE"
#     change_array(calendar)             "NONE"
#     change_array(initial_state)        "default"
#     change_array(fshare)               "0"
#     change_array(oticket)              "0"
#     change_array(s_rt)                 "INFINITY"
#     change_array(h_rt)                 "INFINITY"
#     change_array(s_cpu)                "INFINITY"
#     change_array(h_cpu)                "INFINITY"
#     change_array(s_fsize)              "INFINITY"
#     change_array(h_fsize)              "INFINITY"
#     change_array(s_data)               "INFINITY"
#     change_array(h_data)               "INFINITY"
#     change_array(s_stack)              "INFINITY"
#     change_array(h_stack)              "INFINITY"
#     change_array(s_core)               "INFINITY"
#     change_array(h_core)               "INFINITY"
#     change_array(s_rss)                "INFINITY"
#     change_array(h_rss)                "INFINITY"
#     change_array(s_vmem)               "INFINITY"
#     change_array(h_vmem)               "INFINITY"
#
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue()
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc set_queue { q_name change_array } {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_OUTPUT
  global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_OUTPUT

  upvar $change_array chgar

  if { [get_pe_ckpt_version] > 0 && [info exists chgar(qtype)]} {
     if { [ string match "*CHECKPOINTING*" $chgar(qtype) ] ||
          [ string match "*PARALLEL*" $chgar(qtype) ]   } { 

        set new_chgar_qtype ""
        foreach elem $chgar(qtype) {
           if { [ string match "*CHECKPOINTING*" $elem ] } {
              puts $CHECK_OUTPUT "this qconf version doesn't support CHECKPOINTING value for qtype"
              continue
           } 
           if { [ string match "*PARALLEL*" $elem ] } {
              puts $CHECK_OUTPUT "this qconf version doesn't support PARALLEL value for qtype"
              continue
           } 
           append new_chgar_qtype "$elem "
        } 
        set chgar(qtype) [string trim $new_chgar_qtype]
        puts $CHECK_OUTPUT "using qtype=$chgar(qtype)" 
     }
  }

  puts $CHECK_OUTPUT "setting queue parameters for queue \"$q_name\""
  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {
     if { $elem == "" } {
        continue;
     }
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 
  set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
  set NOT_A_QUEUENAME [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_QUEUE_XISNOTAQUEUENAME_S] $q_name ]
  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $q_name $QUEUE ]
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-mq ${q_name}" $vi_commands $MODIFIED $NOT_A_QUEUENAME]
  if { $result == -2 } {
    add_proc_error "set_queue" -1 "$q_name is not a queue"
  }
  if { $result != 0  } {
    add_proc_error "set_queue" -1 "error modify queue $q_name, $result"
  } 
  return $result
}

#                                                             max. column:     |
#****** sge_procedures/add_queue() ******
# 
#  NAME
#     add_queue -- Add a new queue configuration object
#
#  SYNOPSIS
#     add_queue { change_array {fast_add 0} } 
#
#  FUNCTION
#     Add a new queue configuration object corresponding to the content of 
#     the change_array.
#
#  INPUTS
#     change_array - name of an array variable that will be set by get_config
#     {fast_add 0} - if not 0 the add_queue procedure will use a file for
#                    queue configuration. (faster) (qconf -Aq, not qconf -aq)
#
#  RESULT
#     -1   timeout error
#     -2   queue allready exists
#      0   ok 
#
#  EXAMPLE
#     set new_queue(qname)    "new.q"
#     set new_queue(hostname) "expo1"
#     add_queue new_queue 
#
#  NOTES
#     the array should look like this:
#
#     set change_array(qname) MYHOST
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all valid array names (template queue):
#
#     change_array(qname)                "template"
#     change_array(hostname)             "unknown"
#     change_array(seq_no)               "0"
#     change_array(load_thresholds)      "np_load_avg=1.75"
#     change_array(suspend_thresholds)   "NONE"
#     change_array(nsuspend)             "0"
#     change_array(suspend_interval)     "00:05:00"
#     change_array(priority)             "0"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(notify)               "00:00:60"
#     change_array(owner_list)           "NONE"
#     change_array(user_lists)           "NONE"
#     change_array(xuser_lists)          "NONE"
#     change_array(subordinate_list)     "NONE"
#     change_array(complex_list)         "NONE"
#     change_array(complex_values)       "NONE"
#     change_array(projects)             "NONE"
#     change_array(xprojects)            "NONE"
#     change_array(calendar)             "NONE"
#     change_array(initial_state)        "default"
#     change_array(fshare)               "0"
#     change_array(oticket)              "0"
#     change_array(s_rt)                 "INFINITY"
#     change_array(h_rt)                 "INFINITY"
#     change_array(s_cpu)                "INFINITY"
#     change_array(h_cpu)                "INFINITY"
#     change_array(s_fsize)              "INFINITY"
#     change_array(h_fsize)              "INFINITY"
#     change_array(s_data)               "INFINITY"
#     change_array(h_data)               "INFINITY"
#     change_array(s_stack)              "INFINITY"
#     change_array(h_stack)              "INFINITY"
#     change_array(s_core)               "INFINITY"
#     change_array(h_core)               "INFINITY"
#     change_array(s_rss)                "INFINITY"
#     change_array(h_rss)                "INFINITY"
#     change_array(s_vmem)               "INFINITY"
#     change_array(h_vmem)               "INFINITY"
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue()
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc add_queue { change_array {fast_add 0} } {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_OUTPUT CHECK_TESTSUITE_ROOT CHECK_PRODUCT_TYPE
  global CHECK_USER CHECK_CORE_MASTER CHECK_HOST

  upvar $change_array chgar

  if { [get_pe_ckpt_version] > 0 && [info exists chgar(qtype)]} {
     if { [ string match "*CHECKPOINTING*" $chgar(qtype) ] ||
          [ string match "*PARALLEL*" $chgar(qtype) ]   } { 

        set new_chgar_qtype ""
        foreach elem $chgar(qtype) {
           if { [ string match "*CHECKPOINTING*" $elem ] } {
              puts $CHECK_OUTPUT "this qconf version doesn't support CHECKPOINTING value for qtype"
              continue
           } 
           if { [ string match "*PARALLEL*" $elem ] } {
              puts $CHECK_OUTPUT "this qconf version doesn't support PARALLEL value for qtype"
              continue
           } 
           append new_chgar_qtype "$elem "
        } 
        set chgar(qtype) [string trim $new_chgar_qtype]
        puts $CHECK_OUTPUT "using qtype=$chgar(qtype)" 
     }
  }


  set values [array names chgar]

    if { $fast_add != 0 } {
     # add queue from file!
     set default_array(qname)                "queuename"
     set default_array(hostname)             "hostname"
     set default_array(seq_no)               "0"
     set default_array(load_thresholds)      "np_load_avg=7.00"
     set default_array(suspend_thresholds)   "NONE"
     set default_array(nsuspend)             "1"
     set default_array(suspend_interval)     "00:05:00"
     set default_array(priority)             "0"
     set default_array(min_cpu_interval)     "00:05:00"
     set default_array(processors)           "UNDEFINED"
     if { [get_pe_ckpt_version] > 0 } {
        set default_array(qtype)                "BATCH INTERACTIVE"
        set default_array(pe_list)              "NONE"
        set default_array(ckpt_list)            "NONE"
     } else {
        set default_array(qtype)                "BATCH INTERACTIVE CHECKPOINTING PARALLEL"
     }
     set default_array(rerun)                "FALSE"
     set default_array(slots)                "10"
     set default_array(tmpdir)               "/tmp"
     set default_array(shell)                "/bin/csh"
     set default_array(shell_start_mode)     "NONE"
     set default_array(prolog)               "NONE"
     set default_array(epilog)               "NONE"
     set default_array(starter_method)       "NONE"
     set default_array(suspend_method)       "NONE"
     set default_array(resume_method)        "NONE"
     set default_array(terminate_method)     "NONE"
     set default_array(notify)               "00:00:60"
     set default_array(owner_list)           "NONE"
     set default_array(user_lists)           "NONE"
     set default_array(xuser_lists)          "NONE"
     set default_array(subordinate_list)     "NONE"
     set default_array(complex_list)         "NONE"
     set default_array(complex_values)       "NONE"
     set default_array(calendar)             "NONE"
     set default_array(initial_state)        "default"
     set default_array(s_rt)                 "INFINITY"
     set default_array(h_rt)                 "INFINITY"
     set default_array(s_cpu)                "INFINITY"
     set default_array(h_cpu)                "INFINITY"
     set default_array(s_fsize)              "INFINITY"
     set default_array(h_fsize)              "INFINITY"
     set default_array(s_data)               "INFINITY"
     set default_array(h_data)               "INFINITY"
     set default_array(s_stack)              "INFINITY"
     set default_array(h_stack)              "INFINITY"
     set default_array(s_core)               "INFINITY"
     set default_array(h_core)               "INFINITY"
     set default_array(s_rss)                "INFINITY"
     set default_array(h_rss)                "INFINITY"
     set default_array(s_vmem)               "INFINITY"
     set default_array(h_vmem)               "INFINITY"
  
     if { $CHECK_PRODUCT_TYPE == "sgeee" } {
       set default_array(projects)           "NONE"
       set default_array(xprojects)          "NONE"
       set default_array(fshare)             "0"
       set default_array(oticket)            "0"
     }
  
     foreach elem $values {
        set value [set chgar($elem)]
        puts $CHECK_OUTPUT "--> setting \"$elem\" to \"$value\""
        set default_array($elem) $value
     }
     if {[file isdirectory "$CHECK_TESTSUITE_ROOT/testsuite_trash"] != 1} {
        file mkdir "$CHECK_TESTSUITE_ROOT/testsuite_trash"
     }

     set tmpfile "$CHECK_TESTSUITE_ROOT/testsuite_trash/tmpfile"
     set file [open $tmpfile "w"]
     set values [array names default_array]
     foreach elem $values {
        set value [set default_array($elem)]
        puts $file "$elem                   $value"
     }
     close $file

     set result ""
     set catch_return [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -Aq ${tmpfile}" } result ]
     puts $CHECK_OUTPUT $result
     set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
     set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $default_array(qname) $QUEUE ]

     if { [string match "*$ADDED" $result ] == 0 } {
        add_proc_error "add_queue" "-1" "qconf error or binary not found"
        return
     } 
     return
  }


  set vi_commands "" 
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 

  set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
  set ALREADY_EXISTS [ translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] $QUEUE $chgar(qname)]
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $chgar(qname) $QUEUE ]

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-aq" $vi_commands $ADDED $ALREADY_EXISTS ]  
  if { $result != 0 } {
     add_proc_error "add_queue" -1 "could not add queue [set chgar(qname)] (error: $result)"
  }
  return $result
}


#                                                             max. column:     |
#****** sge_procedures/add_exechost() ******
# 
#  NAME
#     add_exechost -- Add a new exechost configuration object
#
#  SYNOPSIS
#     add_exechost { change_array {fast_add 0} } 
#
#  FUNCTION
#     Add a new execution host configuration object corresponding to the content of 
#     the change_array.
#
#  INPUTS
#     change_array - name of an array variable can contain special settings
#     {fast_add 0} - if not 0 the add_exechost procedure will use a file for
#                    queue configuration. (faster) (qconf -Ae, not qconf -ae)
#
#  RESULT
#     -1   timeout error
#     -2   host allready exists
#      0   ok 
#
#  EXAMPLE
#     set new_host(hostname) "test"
#     add_exechost new_host
#
#  NOTES
#     the array should look like this:
#
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all valid array names (template host):
#
#     change_array(hostname)                    "template"
#     change_array(load_scaling)                "NONE"
#     change_array(complex_list)                "NONE"
#     change_array(complex_values)              "NONE"
#     change_array(user_lists)                  "NONE"
#     change_array(xuser_lists)                 "NONE"
#
#     additional names for an enterprise edition system:
#     change_array(projects)                    "NONE"
#     change_array(xprojects)                   "NONE"
#     change_array(usage_scaling)               "NONE"
#     change_array(resource_capability_factor)  "0.000000"
#*******************************
proc add_exechost { change_array {fast_add 0} } {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_OUTPUT CHECK_TESTSUITE_ROOT CHECK_PRODUCT_TYPE
  global CHECK_CORE_MASTER

  upvar $change_array chgar
  set values [array names chgar]

    if { $fast_add != 0 } {
     # add queue from file!
     set default_array(hostname)          "template"
     set default_array(load_scaling)      "NONE"
     set default_array(complex_list)      "NONE"
     set default_array(complex_values)    "NONE"
     set default_array(user_lists)        "NONE"
     set default_array(xuser_lists)       "NONE"
  
     if { $CHECK_PRODUCT_TYPE == "sgeee" } {
       set default_array(projects)                    "NONE"
       set default_array(xprojects)                   "NONE"
       set default_array(usage_scaling)               "NONE"
       set default_array(resource_capability_factor)  "0.000000"
     }
  
     foreach elem $values {
        set value [set chgar($elem)]
        puts $CHECK_OUTPUT "--> setting \"$elem\" to \"$value\""
        set default_array($elem) $value
     }

     if {[file isdirectory "$CHECK_TESTSUITE_ROOT/testsuite_trash"] != 1} {
        file mkdir "$CHECK_TESTSUITE_ROOT/testsuite_trash"
     }

     set tmpfile "$CHECK_TESTSUITE_ROOT/testsuite_trash/tmpfile"
     set file [open $tmpfile "w"]
     set values [array names default_array]
     foreach elem $values {
        set value [set default_array($elem)]
        puts $file "$elem                   $value"
     }
     close $file

     set result ""
     set catch_return [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -Ae ${tmpfile}" } result ]
     puts $CHECK_OUTPUT $result
     set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S] "*"

     if { [string match "*$ADDED" $result] == 0 } {
        add_proc_error "add_exechost" "-1" "qconf error or binary not found"
        return
     }
     return
  }


  set vi_commands "" 
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 

  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*" ]
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S] "*"

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ae" $vi_commands $ADDED $ALREADY_EXISTS ]  
  if { $result != 0 } {
     add_proc_error "add_exechost" -1 "could not add queue [set chgar(qname)] (error: $result)"
  }
  return $result
}

#****** sge_procedures/get_scheduling_info() ***********************************
#  NAME
#     get_scheduling_info() -- get scheduling information 
#
#  SYNOPSIS
#     get_scheduling_info { job_id { check_pending 1 } } 
#
#  FUNCTION
#     This procedure starts the get_qstat_j_info() procedure and returns
#     the "scheduling info" value. The procedure returns ALLWAYS an guilty
#     text string.
#
#  INPUTS
#     job_id              - job id
#     { check_pending 1 } - 1(default): do a wait_forjob_pending first
#                           0         : no wait_for_jobpending() call         
#
#  RESULT
#     scheduling info text
#
#  SEE ALSO
#     sge_procedures/get_qstat_j_info()
#     sge_procedures/wait_forjob_pending()
# 
#*******************************************************************************
proc get_scheduling_info { job_id { check_pending 1 }} {
   global CHECK_OUTPUT CHECK_ARCH CHECK_PRODUCT_ROOT CHECK_CORE_MASTER

   if { $check_pending == 1 } {
      set result [ wait_for_jobpending $job_id "leeper" 120 ]
      if { $result != 0 } {
         return "job not pending"
      }
   }
   puts $CHECK_OUTPUT "Trigger scheduler monitoring"
   catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-tsm" } catch_result
   puts $CHECK_OUTPUT $catch_result
   # timeout 120

   set my_timeout [ expr ( [timestamp] + 30 ) ] 
   while { 1 } {
      if { [get_qstat_j_info $job_id ] } {
         set help_var_name "scheduling info" 
         if { [info exists qstat_j_info($help_var_name)] } {
            set sched_info $qstat_j_info($help_var_name)
         } else {
            set sched_info "no messages available"
         }
         set help_var_name [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SCHEDD_SCHEDULINGINFO]] 
         if { [info exists qstat_j_info($help_var_name)] } {
            set sched_info $qstat_j_info($help_var_name)
         } else {
            set sched_info "no messages available"
         }


         set INFO_NOMESSAGE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SCHEDD_INFO_NOMESSAGE]]

         if { [string first "no messages available" $sched_info] < 0 && [string first $INFO_NOMESSAGE $sched_info] < 0 } {
            puts $CHECK_OUTPUT $sched_info
            return $sched_info
         }
      }
      puts $CHECK_OUTPUT "waiting for scheduling info information ..."
      sleep 2
      if { [timestamp] > $my_timeout } {
         return "timeout"
      }
   }
}


#****** sge_procedures/add_access_list() ***************************************
#  NAME
#     add_access_list() -- add user access list
#
#  SYNOPSIS
#     add_access_list { user_array list_name } 
#
#  FUNCTION
#     This procedure starts the qconf -au command to add a new user access list.
#
#  INPUTS
#     user_array - tcl array with user names
#     list_name  - name of the new list
#
#  RESULT
#     -1 on error, 0 on success
#
#  SEE ALSO
#     sge_procedures/del_access_list()
#
#*******************************************************************************
proc add_access_list { user_array list_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT CHECK_CORE_MASTER 

  set arguments ""
  foreach elem $user_array {
     append arguments "$elem,"
  }
  append arguments " $list_name"

  set result ""
  set catch_return [ catch {  
      eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -au $arguments" 
  } result ]
  puts $CHECK_OUTPUT $result
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_GDI_ADDTOACL_SS ] $user_array $list_name]
  if { [string first "added" $result ] < 0 && [string first $ADDED $result ] < 0 } {
     add_proc_error "add_access_list" "-1" "could not add access_list $list_name"
     return -1
  }
  return 0
}

#****** sge_procedures/del_access_list() ***************************************
#  NAME
#     del_access_list() -- delete user access list
#
#  SYNOPSIS
#     del_access_list { list_name } 
#
#  FUNCTION
#     This procedure starts the qconf -dul command to delete a user access
#     list.
#
#  INPUTS
#     list_name - name of access list to delete
#
#  RESULT
#     -1 on error, 0 on success
#
#  SEE ALSO
#     sge_procedures/add_access_list()
# 
#*******************************************************************************
proc del_access_list { list_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT
  global CHECK_CORE_MASTER CHECK_USER CHECK_HOST

  set result ""
  set catch_return [ catch {  
      eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -dul $list_name" 
  } result ]
   puts $CHECK_OUTPUT $result
  set USER [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_USERSET]]
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $list_name $USER]
  puts $CHECK_OUTPUT $REMOVED

  if { [ string match "*$REMOVED" $result ] == 0 } {
     add_proc_error "add_access_list" "-1" "could not delete access_list $list_name"
     return -1
  }
  return 0
}


#                                                             max. column:     |
#****** sge_procedures/del_queue() ******
# 
#  NAME
#     del_queue -- delete a queue
#
#  SYNOPSIS
#     del_queue { q_name } 
#
#  FUNCTION
#     remove a queue from the qmaster configuration
#
#  INPUTS
#     q_name - name of the queue to delete
#
#  RESULT
#     0  : ok
#     -1 : timeout error
#
#  EXAMPLE
#     del_queue "my_own_queue.q"
#
#  NOTES
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc del_queue { q_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT CHECK_HOST

  set result ""
  set catch_return [ catch {  
      eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -dq ${q_name}" 
  } result ]

  set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $q_name $QUEUE ]

  if { [string match "*$REMOVED" $result ] == 0 } {
     add_proc_error "del_queue" "-1" "could not delete queue $q_name"
     return -1
  } 
  return 0
}

#                                                             max. column:     |
#****** sge_procedures/get_queue() ******
# 
#  NAME
#     get_queue -- get queue configuration information
#
#  SYNOPSIS
#     get_queue { q_name change_array } 
#
#  FUNCTION
#     Get the actual configuration settings for the named queue
#
#  INPUTS
#     q_name       - name of the queue
#     change_array - name of an array variable that will get set by get_config
#
#  EXAMPLE
#     get_queue "myqueue.q" qinfo
#     puts qinfo(seq_no) 
#
#  NOTES
#     the array should look like this:
#
#     set change_array(qname) MYHOST
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all guilty array names (template queue):
#
#     change_array(qname)                "template"
#     change_array(hostname)             "unknown"
#     change_array(seq_no)               "0"
#     change_array(load_thresholds)      "np_load_avg=1.75"
#     change_array(suspend_thresholds)   "NONE"
#     change_array(nsuspend)             "0"
#     change_array(suspend_interval)     "00:05:00"
#     change_array(priority)             "0"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(notify)               "00:00:60"
#     change_array(owner_list)           "NONE"
#     change_array(user_lists)           "NONE"
#     change_array(xuser_lists)          "NONE"
#     change_array(subordinate_list)     "NONE"
#     change_array(complex_list)         "NONE"
#     change_array(complex_values)       "NONE"
#     change_array(projects)             "NONE"
#     change_array(xprojects)            "NONE"
#     change_array(calendar)             "NONE"
#     change_array(initial_state)        "default"
#     change_array(fshare)               "0"
#     change_array(oticket)              "0"
#     change_array(s_rt)                 "INFINITY"
#     change_array(h_rt)                 "INFINITY"
#     change_array(s_cpu)                "INFINITY"
#     change_array(h_cpu)                "INFINITY"
#     change_array(s_fsize)              "INFINITY"
#     change_array(h_fsize)              "INFINITY"
#     change_array(s_data)               "INFINITY"
#     change_array(h_data)               "INFINITY"
#     change_array(s_stack)              "INFINITY"
#     change_array(h_stack)              "INFINITY"
#     change_array(s_core)               "INFINITY"
#     change_array(h_core)               "INFINITY"
#     change_array(s_rss)                "INFINITY"
#     change_array(h_rss)                "INFINITY"
#     change_array(s_vmem)               "INFINITY"
#     change_array(h_vmem)               "INFINITY"
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc get_queue { q_name change_array } {


  global CHECK_PRODUCT_ROOT CHECK_ARCH
  upvar $change_array chgar

  set catch_return [ catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -sq ${q_name}" } result ]
  if { $catch_return != 0 } {
     add_proc_error "get_queue" "-1" "qconf error or binary not found"
     return
  }

  # split each line as listelement
  set help [split $result "\n"]

  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     if { $id != "" } {
        set chgar($id) $value
     }
  }
}

#                                                             max. column:     |
#****** sge_procedures/suspend_queue() ******
# 
#  NAME
#     suspend_queue -- set a queue in suspend mode
#
#  SYNOPSIS
#     suspend_queue { qname } 
#
#  FUNCTION
#     This procedure will set the given queue into suspend state
#
#  INPUTS
#     qname - name of the queue to suspend 
#
#  RESULT
#     0  - ok
#    -1  - error 
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc suspend_queue { qname } {
 global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_HOST CHECK_USER
 global CHECK_OUTPUT
  log_user 0 
  set WAS_SUSPENDED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_SUSPENDQ_SSS] "*" "*" "*" ]

  
  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod"
  set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-s $qname" ]
  set sp_id [ lindex $sid 1 ]
  set result -1	

  log_user 0
  set timeout 30
  expect {
     -i $sp_id full_buffer {
         set result -1
         add_proc_error "suspend_queue" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
     }
     -i $sp_id "was suspended" {
         set result 0
     }
      -i $sp_id $WAS_SUSPENDED {
         set result 0
     }

	  -i $sp_id default {
         puts $CHECK_OUTPUT $expect_out(buffer)
	      set result -1
	  }
  }
  # close spawned process 
  close_spawn_process $sid
  log_user 1
  if { $result != 0 } {
     add_proc_error "suspend_queue" -1 "could not suspend queue \"$qname\""
  }

  return $result
}

#                                                             max. column:     |
#****** sge_procedures/unsuspend_queue() ******
# 
#  NAME
#     unsuspend_queue -- set a queue in suspend mode
#
#  SYNOPSIS
#     unsuspend_queue { queue } 
#
#  FUNCTION
#     This procedure will set the given queue into unsuspend state
#
#  INPUTS
#     queue - name of the queue to set into unsuspend state
#
#  RESULT
#     0  - ok
#    -1  - error 
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc unsuspend_queue { queue } {
   global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_HOST CHECK_USER
   global CHECK_OUTPUT

  set timeout 30
  log_user 0 
   
  set UNSUSP_QUEUE [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_UNSUSPENDQ_SSS] "*" "*" "*" ]

  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod"
  set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-us $queue" ]     
  set sp_id [ lindex $sid 1 ]
  set result -1	
  log_user 0 

  set timeout 30
  expect {
      -i $sp_id full_buffer {
         set result -1
         add_proc_error "unsuspend_queue" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
      }
      -i $sp_id "unsuspended queue" {
         set result 0 
      }
      -i $sp_id  $UNSUSP_QUEUE {
         set result 0 
      }
      -i $sp_id default {
         puts $CHECK_OUTPUT $expect_out(buffer) 
         set result -1 
      }
  }
  # close spawned process 
  close_spawn_process $sid
  log_user 1   
  if { $result != 0 } {
     add_proc_error "unsuspend_queue" -1 "could not unsuspend queue \"$queue\""
  }
  return $result
}

#                                                             max. column:     |
#****** sge_procedures/disable_queue() ******
# 
#  NAME
#     disable_queue -- disable queues
#
#  SYNOPSIS
#     disable_queue { queue } 
#
#  FUNCTION
#     Disable the given queue/queue list
#
#  INPUTS
#     queue - name of queues to disable
#
#  RESULT
#     0  - ok
#    -1  - error
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc disable_queue { queuelist } {
 global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer 
  global CHECK_OUTPUT CHECK_HOST CHECK_USER
  global CHECK_CORE_MASTER CHECK_USER
  
  set return_value ""
  # spawn process

  set nr_of_queues 0
  set nr_disabled 0

  foreach elem $queuelist {
     set queue_name($nr_of_queues) $elem
     incr nr_of_queues 1
  }

  set queue_nr 0
  while { $queue_nr != $nr_of_queues } {
     log_user 0
     set queues ""
     set i 10  ;# maximum 10 queues at one time
     while { $i > 0 } {
        if { $queue_nr < $nr_of_queues } {
           append queues " $queue_name($queue_nr)"
           incr queue_nr 1
        }
        incr i -1
     }   
     
     set result ""
     set catch_return [ catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod -d $queues" } result ]
     debug_puts $CHECK_OUTPUT "disable queue(s) $queues"
     set res_split [ split $result "\n" ]   
     foreach elem $res_split {
        if { [ string first "has been disabled" $result ] >= 0 } {
           incr nr_disabled 1 
        } else {
           # try to find localized output
           foreach q_name $queues {
              set HAS_DISABLED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_DISABLEQ_SSS] $q_name $CHECK_USER "*" ]
               
              if { [ string match "*$HAS_DISABLED" $result ] } {
                 incr nr_disabled 1
                 break
              } 
           }
        }
     }
  }    

  if { $nr_of_queues != $nr_disabled } {
     add_proc_error "disable_queue" "-1" "could not disable all queues"
     return -1
  }
 
  return 0
}


#                                                             max. column:     |
#****** sge_procedures/enable_queue() ******
# 
#  NAME
#     enable_queue -- enable queuelist
#
#  SYNOPSIS
#     enable_queue { queue } 
#
#  FUNCTION
#     This procedure enables a given queuelist by calling the qmod -e binary
#
#  INPUTS
#     queue - name of queues to enable (list)
#
#  RESULT
#     0  - ok
#    -1  - on error
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc enable_queue { queuelist } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer 
  global CHECK_OUTPUT CHECK_HOST CHECK_USER CHECK_CORE_MASTER 
  
  set return_value ""
  # spawn process

  set nr_of_queues 0
  set nr_enabled 0

  foreach elem $queuelist {
     set queue_name($nr_of_queues) $elem
     incr nr_of_queues 1
  }

  set queue_nr 0
  while { $queue_nr != $nr_of_queues } {
     log_user 0
     set queues ""
     set i 10  ;# maximum 10 queues at one time
     while { $i > 0 } {
        if { $queue_nr < $nr_of_queues } {
           append queues " $queue_name($queue_nr)"
           incr queue_nr 1
        }
        incr i -1
     }   
     set result ""
     set catch_return [ catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod -e $queues" } result ]
     debug_puts $CHECK_OUTPUT "enable queue(s) $queues"
     set res_split [ split $result "\n" ]   
     foreach elem $res_split {
        if { [ string first "has been enabled" $result ] >= 0 } {
           incr nr_enabled 1 
        } else {
           # try to find localized output
           foreach q_name $queues {
              set BEEN_ENABLED  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_ENABLEQ_SSS] $q_name $CHECK_USER "*" ]
              if { [ string match "*$BEEN_ENABLED" $result ] } {
                 incr nr_enabled 1
                 break
              } 
           }
        }
     }
  }    

  if { $nr_of_queues != $nr_enabled } {
     add_proc_error "enable_queue" "-1" "could not enable all queues"
     return -1
  }
  return 0
}


#                                                             max. column:     |
#****** sge_procedures/get_queue_state() ******
# 
#  NAME
#     get_queue_state -- get the state of a queue
#
#  SYNOPSIS
#     get_queue_state { queue } 
#
#  FUNCTION
#     This procedure returns the state of the queue by parsing output of qstat -f. 
#
#  INPUTS
#     queue - name of the queue
#
#  RESULT
#     The return value can contain more than one state. Here is a list of possible
#     states:
#
#     u(nknown)
#     a(larm)
#     A(larm)
#     C(alendar  suspended)
#     s(uspended)
#     S(ubordinate)
#     d(isabled)
#     D(isabled)
#     E(rror)
#
#*******************************
proc get_queue_state { queue } {

  global CHECK_PRODUCT_ROOT CHECK_ARCH

  set catch_return [ catch { 
     eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat -f -q $queue" 
  } result ]

  if { $catch_return != 0 } {
     add_proc_error "get_queue_state" "-1" "qstat error or binary not found"
     return ""
  }

  # split each line as listelement
  set back ""
  set help [split $result "\n"]
  foreach line $help { 
      if { [ string compare [lindex $line 0] $queue ] == 0 } {
         set back [lindex $line 5 ]
         return $back
      }
  }

  add_proc_error "get_queue_state" -1 "queue \"$queue\" not found" 
  return ""
}



#                                                             max. column:     |
#****** sge_procedures/add_checkpointobj() ******
# 
#  NAME
#     add_checkpointobj -- add a new checkpoint definiton object
#
#  SYNOPSIS
#     add_checkpointobj { change_array } 
#
#  FUNCTION
#     This procedure will add a new checkpoint definition object 
#
#  INPUTS
#     change_array - name of an array variable that will be set by 
#                    add_checkpointobj
#
#  NOTES
#     The array should look like follows:
#     
#     set myarray(ckpt_name) "myname"
#     set myarray(queue_list) "big.q"
#     ...
#
#     Here the possbile change_array values with some typical settings:
# 
#     ckpt_name          test
#     interface          userdefined
#     ckpt_command       none
#     migr_command       none
#     restart_command    none
#     clean_command      none
#     ckpt_dir           /tmp
#     queue_list         NONE
#     signal             none
#     when               sx
#
#  RESULT
#      0  - ok
#     -1  - timeout error
#     -2  - object already exists
#     -3  - queue reference does not exist
# 
#  SEE ALSO
#     sge_procedures/del_checkpointobj()
#*******************************
proc add_checkpointobj { change_array } {

  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_USER CHECK_CORE_MASTER CHECK_HOST CHECK_OUTPUT

  upvar $change_array chgar

  if { [get_pe_ckpt_version] > 0 } {
     if { [ info exists chgar(queue_list) ] } { 
        puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for ckpt objects"
        add_proc_error "add_checkpointobj" -3 "this qconf version doesn't support queue_list for ckpt objects,\nuse assign_queues_with_ckpt_object() after adding checkpoint\nobjects and don't use queue_list parameter.\nyou can call get_pe_ckpt_version() to test ckpt version"
        unset chgar(queue_list)
     }
  }

  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  }

  set my_ckpt_name [set chgar(ckpt_name)]
  set my_args "-ackpt $my_ckpt_name"
 
  set ALREADY_EXISTS [ translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" $chgar(ckpt_name)]
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $chgar(ckpt_name) "checkpoint interface" ]
  set REFERENCED_IN_QUEUE_LIST_OF_CHECKPOINT [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNQUEUE_SSSS] "*" "*" "*" "*"] 

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" $my_args $vi_commands $ADDED $ALREADY_EXISTS $REFERENCED_IN_QUEUE_LIST_OF_CHECKPOINT ] 
  
  if { $result == -1 } { add_proc_error "add_checkpointobj" -1 "timeout error" }
  if { $result == -2 } { add_proc_error "add_checkpointobj" -1 "already exists" }
  if { $result == -3 } { add_proc_error "add_checkpointobj" -1 "queue reference does not exist" }
  if { $result != 0  } { add_proc_error "add_checkpointobj" -1 "could nod add checkpoint object" }

  return $result
}


#****** sge_procedures/set_checkpointobj() *************************************
#  NAME
#     set_checkpointobj() -- set or change checkpoint object configuration
#
#  SYNOPSIS
#     set_checkpointobj { ckpt_obj change_array } 
#
#  FUNCTION
#     Set a checkpoint configuration corresponding to the content of the 
#     change_array.
#
#  INPUTS
#     ckpt_obj     - name of the checkpoint object to configure
#     change_array - name of array variable that will be set by 
#                    set_checkpointobj()
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  SEE ALSO
#     sge_procedures/get_checkpointobj()
#*******************************************************************************
proc set_checkpointobj { ckpt_obj change_array } {

  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_USER CHECK_CORE_MASTER CHECK_HOST CHECK_OUTPUT

  upvar $change_array chgar

  if { [get_pe_ckpt_version] > 0 && [info exists chgar(queue_list)]} {
     if { [ info exists chgar(queue_list) ] } { 
        puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for ckpt objects"
        add_proc_error "set_checkpointobj" -3 "this qconf version doesn't support queue_list for ckpt objects,\nuse assign_queues_with_ckpt_object() after adding checkpoint\nobjects and don't use queue_list parameter.\nyou can call get_pe_ckpt_version() to test ckpt version"
        unset chgar(queue_list)
     }
  }

  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  }

  set my_args "-mckpt $ckpt_obj"
 
  set ALREADY_EXISTS [ translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" $ckpt_obj]
  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $ckpt_obj "checkpoint interface" ]

  set REFERENCED_IN_QUEUE_LIST_OF_CHECKPOINT [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNQUEUE_SSSS] "*" "*" "*" "*"] 

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" $my_args $vi_commands $MODIFIED $ALREADY_EXISTS $REFERENCED_IN_QUEUE_LIST_OF_CHECKPOINT ] 
  
  if { $result == -1 } { add_proc_error "add_checkpointobj" -1 "timeout error" }
  if { $result == -2 } { add_proc_error "add_checkpointobj" -1 "already exists" }
  if { $result == -3 } { add_proc_error "add_checkpointobj" -1 "queue reference does not exist" }
  if { $result != 0  } { add_proc_error "add_checkpointobj" -1 "could nod modify checkpoint object" }

  return $result
}


#                                                             max. column:     |
#****** sge_procedures/del_checkpointobj() ******
# 
#  NAME
#     del_checkpointobj -- delete checkpoint object definition
#
#  SYNOPSIS
#     del_checkpointobj { checkpoint_name } 
#
#  FUNCTION
#     This procedure will delete a checkpoint object definition by its name.
#
#  INPUTS
#     checkpoint_name - name of the checkpoint object
#
#  RESULT
#      0  - ok
#     -1  - timeout error
#
#  SEE ALSO
#     sge_procedures/add_checkpointobj()
#*******************************
proc del_checkpointobj { checkpoint_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_CORE_MASTER CHECK_USER CHECK_HOST
  global CHECK_OUTPUT

  if { [get_pe_ckpt_version] > 0 } {
     puts $CHECK_OUTPUT "searching for references in all defined queues ..."
     set NO_QUEUE_DEFINED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "queue"]
     catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sql" } result
     if { [string first $NO_QUEUE_DEFINED $result] >= 0 } {
        puts $CHECK_OUTPUT "no queue defined"
     } else {
        foreach elem $result {
           puts $CHECK_OUTPUT "queue: $elem"
           if {[info exists params]} {
              unset params
           }
           get_queue $elem params
           if { [string first $checkpoint_name $params(ckpt_list)] >= 0 } {
              puts $CHECK_OUTPUT "ckpt obj $checkpoint_name is referenced in queue $elem, removing entry."
              set new_params ""
              set help_list [split $params(ckpt_list) ","]
              set help_list2 ""
              foreach element $help_list {
                 append help_list2 "$element "
              }
              set help_list [string trim $help_list2] 
              foreach help_ckpt $help_list {
                 if { [string match $checkpoint_name $help_ckpt] != 1 } {
                    append new_params "$help_ckpt "
                 }
              }
              if { [llength $new_params ] == 0 } {
                 set new_params "NONE"
              }
              set mod_params(ckpt_list) [string trim $new_params]
              set_queue $elem mod_params 
           }
        }
     }
  }
  
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $checkpoint_name "*" ]

  log_user 0 
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dckpt $checkpoint_name"  ]
  set sp_id [ lindex $id 1 ]
  set timeout 30
  set result -1 
  	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_checkpointobj" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id $REMOVED {
      set result 0
    }
    -i $sp_id "removed" {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
    
  }
  close_spawn_process $id
  log_user 1

  if { $result != 0 } {
     add_proc_error "del_checkpointobj" -1 "could not delete checkpoint object $checkpoint_name"
  } 

  return $result
}



#                                                             max. column:     |
#****** sge_procedures/add_pe() ******
# 
#  NAME
#     add_pe -- add new parallel environment definition object
#
#  SYNOPSIS
#     add_pe { change_array { version_check 1 } } 
#
#  FUNCTION
#     This procedure will create a new pe (parallel environemnt) definition 
#     object.
#
#  INPUTS
#     change_array - name of an array variable that will be set by add_pe
#     { version_check 1 } - (default 1): check pe/ckpt version
#                           (0): don't check pe/ckpt version
#
#  RESULT
#      0 - ok
#     -1 - timeout error
#     -2 - pe already exists
#     -3 - could not add pe
#
#  EXAMPLE
#     set mype(pe_name) "mype"
#     set mype(user_list) "user1"
#     add_pe pe_name
#
#  NOTES
#     The array should look like this:
#
#     set change_array(pe_name) 	"mype"
#     set change_array(user_list) 	"crei"
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#
#     pe_name           testpe
#     queue_list        NONE
#     slots             0
#     user_lists        NONE
#     xuser_lists       NONE
#     start_proc_args   /bin/true
#     stop_proc_args    /bin/true
#     allocation_rule   $pe_slots
#     control_slaves    FALSE
#     job_is_first_task TRUE
#
#
#  SEE ALSO
#     sge_procedures/del_pe()
#*******************************
proc add_pe { change_array { version_check 1 } } {
# pe_name           testpe
# queue_list        NONE
# slots             0
# user_lists        NONE
# xuser_lists       NONE
# start_proc_args   /bin/true
# stop_proc_args    /bin/true
# allocation_rule   $pe_slots
# control_slaves    FALSE
# job_is_first_task TRUE


  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

  upvar $change_array chgar

  if { $version_check == 1 } {
     if { [get_pe_ckpt_version] > 0 && [info exists chgar(queue_list)]} {
        if { [ info exists chgar(queue_list) ] } { 
           puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for pe objects"
           add_proc_error "add_pe" -3 "this qconf version doesn't support queue_list for pe objects,\nuse assign_queues_with_pe_object() after adding pe\nobjects and don't use queue_list parameter.\nyou can call get_pe_ckpt_version() to test pe version"
           unset chgar(queue_list)
        }
     }
  }


  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 

  set ADDED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*" ]
  set NOT_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNUSERSET_SSSS] "*" "*" "*" "*" ]


  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ap [set chgar(pe_name)]" $vi_commands $ADDED $ALREADY_EXISTS $NOT_EXISTS ]
  
  if {$result == -1 } { add_proc_error "add_pe" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_pe" -1 "parallel environment \"[set chgar(pe_name)]\" already exists" }
  if {$result == -3 } { add_proc_error "add_pe" -1 "something (perhaps a queue) does not exist" }
  if {$result != 0  } { add_proc_error "add_pe" -1 "could not add parallel environment \"[set chgar(pe_name)]\"" }

  return $result
}


#****** sge_procedures/set_pe() ************************************************
#  NAME
#     set_pe() -- set or change pe configuration
#
#  SYNOPSIS
#     set_pe { pe_obj change_array } 
#
#  FUNCTION
#     Set a pe configuration corresponding to the content of the change_array.
#
#  INPUTS
#     pe_obj       - name of pe object to configure
#     change_array - name of an array variable that will be set by set_pe()
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  SEE ALSO
#     sge_procedures/add_pe()
#*******************************************************************************
proc set_pe { pe_obj change_array } {
# pe_name           testpe
# queue_list        NONE
# slots             0
# user_lists        NONE
# xuser_lists       NONE
# start_proc_args   /bin/true
# stop_proc_args    /bin/true
# allocation_rule   $pe_slots
# control_slaves    FALSE
# job_is_first_task TRUE


  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

  upvar $change_array chgar



  if { [get_pe_ckpt_version] > 0 && [info exists chgar(queue_list)]} {
     if { [ info exists chgar(queue_list) ] } { 
        puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for pe objects"
        add_proc_error "set_pe" -3 "this qconf version doesn't support queue_list for pe objects,\nuse assign_queues_with_pe_object() after adding pe\nobjects and don't use queue_list parameter.\nyou can call get_pe_ckpt_version() to test pe version"
        unset chgar(queue_list)
     }
  }

  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 

  set MODIFIED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*" ]
  set NOT_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNUSERSET_SSSS] "*" "*" "*" "*" ]


  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-mp $pe_obj" $vi_commands $MODIFIED $ALREADY_EXISTS $NOT_EXISTS ]
  
  if {$result == -1 } { add_proc_error "set_pe" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "set_pe" -1 "parallel environment \"$pe_obj\" already exists" }
  if {$result == -3 } { add_proc_error "set_pe" -1 "something (perhaps a queue) does not exist" }
  if {$result != 0  } { add_proc_error "set_pe" -1 "could not add parallel environment \"$pe_obj\"" }

  return $result
}


#                                                             max. column:     |
#****** sge_procedures/add_user() ******
# 
#  NAME
#     add_user -- ??? 
#
#  SYNOPSIS
#     add_user { change_array } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     change_array - ??? 
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
proc add_user { change_array { from_file 0 } } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_PRODUCT_TYPE
  global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_USER CHECK_OUTPUT
  upvar $change_array chgar
  set values [array names chgar]

  if { [ string compare $CHECK_PRODUCT_TYPE "sge" ] == 0 } {
     add_proc_error "add_user" -1 "not possible for sge systems"
     return -3
  }
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]

  if { $from_file != 0 } {
     set f_name [get_tmp_file_name]
     set fd [open $f_name "w"]
     set org_struct(name) "template"
     set org_struct(oticket) "0"
     set org_struct(fshare) "0"
     set org_struct(default_project) "NONE"
     foreach elem $values {
        set org_struct($elem) $chgar($elem)
     }
     set ogr_struct_names [array names org_struct]
     foreach elem  $ogr_struct_names {
        puts $CHECK_OUTPUT "$elem $org_struct($elem)"
        puts $fd "$elem $org_struct($elem)"
     }
     close $fd 
     puts $CHECK_OUTPUT "using file $f_name"
     set result [start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-Auser $f_name"]
     if { $prg_exit_state != 0 } {
        add_proc_error "add_user" -1 "error running qconf -Auser"
     }
     set result [string trim $result]
     puts $CHECK_OUTPUT "\"$result\""
#     puts $CHECK_OUTPUT "\"$ADDED\"" 
     if { [ string match "*$ADDED" $result] } {
        return 0
     }
     if { [ string match "*$ALREADY_EXISTS" $result] } {
        add_proc_error "add_user" -1 "\"[set chgar(name)]\" already exists"
        return -2
     }
     add_proc_error "add_user" -1 "\"error adding [set chgar(name)]\""
     return -100
  }

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-auser" $vi_commands $ADDED $ALREADY_EXISTS ]
  
  if {$result == -1 } { add_proc_error "add_user" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_user" -1 "\"[set chgar(name)]\" already exists" }
  if {$result != 0  } { add_proc_error "add_user" -1 "could not add user \"[set chgar(name)]\"" }

  return $result
}




#****** sge_procedures/mod_user() **********************************************
#  NAME
#     mod_user() -- ??? 
#
#  SYNOPSIS
#     mod_user { change_array { from_file 0 } } 
#
#  FUNCTION
#     modify user with qconf -muser or -Muser
#
#  INPUTS
#     change_array    - array name with settings to modifiy
#                       (e.g. set my_settings(default_project) NONE )
#                       -> array name "name" must be set (for username)
#
#     { from_file 0 } - if 0: use -mconf , else use -Mconf
#
#  RESULT
#     0 on success
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc mod_user { change_array { from_file 0 } } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_PRODUCT_TYPE
  global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_USER CHECK_OUTPUT
  upvar $change_array chgar
  set values [array names chgar]

  if { [ string compare $CHECK_PRODUCT_TYPE "sge" ] == 0 } {
     add_proc_error "mod_user" -1 "not possible for sge systems"
     return -3
  }
  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS ] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]

  if { $from_file != 0 } {
     set orginal_settings [ start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-suser $chgar(name)" ]
     if { $prg_exit_state != 0 } {
        add_proc_error "mod_user" -1 "\"error modify user [set chgar(name)]\""
        return -100
     }

     if { [string first "default_project" $orginal_settings] < 0  } {
         add_proc_error "mod_user" -1 "\"error modify user [set chgar(name)]\""
        return -100
     }

     set orginal_settings [split $orginal_settings "\n"]

     set config_elements "name oticket fshare default_project"
     foreach elem $config_elements {
        foreach line $orginal_settings {
           set line [string trim $line]
           if { [ string compare $elem [lindex $line 0] ] == 0 } {
              set org_struct($elem) [lrange $line 1 end]
           }
        }
     }
     set ogr_struct_names [array names org_struct]
     foreach elem $ogr_struct_names {
        puts $CHECK_OUTPUT ">$elem=$org_struct($elem)<"
     }
     set f_name [get_tmp_file_name]
     set fd [open $f_name "w"]
     foreach elem $values {
        set org_struct($elem) $chgar($elem)
     }
     set ogr_struct_names [array names org_struct]
     foreach elem  $ogr_struct_names {
        puts $CHECK_OUTPUT "$elem $org_struct($elem)"
        puts $fd "$elem $org_struct($elem)"
     }
     close $fd 
     puts $CHECK_OUTPUT "using file $f_name"
     set result [start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-Muser $f_name"]
     if { $prg_exit_state != 0 } {
        add_proc_error "mod_user" -1 "error running qconf -Auser"
     }
     set result [string trim $result]
     puts $CHECK_OUTPUT "\"$result\""
     puts $CHECK_OUTPUT "\"$MODIFIED\"" 
     if { [ string match "*$MODIFIED" $result] } {
        return 0
     }
     if { [ string match "*$ALREADY_EXISTS" $result] } {
        add_proc_error "mod_user" -1 "\"[set chgar(name)]\" already exists"
        return -2
     }
     add_proc_error "mod_user" -1 "\"modifiy user error  [set chgar(name)]\""
     return -100
  }

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-muser $chgar(name)" $vi_commands $MODIFIED $ALREADY_EXISTS ]
  
  if {$result == -1 } { add_proc_error "mod_user" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "mod_user" -1 "\"[set chgar(name)]\" already exists" }
  if {$result != 0  } { add_proc_error "mod_user" -1 "could not mod user \"[set chgar(name)]\"" }

  return $result
}


#                                                             max. column:     |
#****** sge_procedures/add_prj() ******
# 
#  NAME
#     add_prj -- ??? 
#
#  SYNOPSIS
#     add_prj { change_array } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     change_array - ??? 
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
proc add_prj { change_array } {

# returns 
# -100 on unknown error
# -1   on timeout
# -2   if queue allready exists
# 0    if ok

# name      template
# oticket   0
# fshare    0
# acl       NONE
# xacl      NONE  
# 
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_PRODUCT_TYPE
  global CHECK_CORE_MASTER CHECK_USER

  upvar $change_array chgar
  set values [array names chgar]

  if { [ string compare $CHECK_PRODUCT_TYPE "sge" ] == 0 } {
     add_proc_error "add_prj" -1 "not possible for sge systems"
     return
  }


  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 
#  set PROJECT [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PROJECT]]
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-aprj" $vi_commands $ADDED $ALREADY_EXISTS ]
  
  if {$result == -1 } { add_proc_error "add_prj" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_prj" -1 "\"[set chgar(name)]\" already exists" }
  if {$result != 0  } { add_proc_error "add_prj" -1 "could not add project \"[set chgar(name)]\"" }

  return $result
}


#                                                             max. column:     |
#****** sge_procedures/del_pe() ******
# 
#  NAME
#     del_pe -- delete parallel environment object definition
#
#  SYNOPSIS
#     del_pe { mype_name } 
#
#  FUNCTION
#     This procedure will delete a existing parallel environment, defined with
#     sge_procedures/add_pe.
#
#  INPUTS
#     mype_name - name of parallel environment to delete
#
#  RESULT
#     0  - ok
#     -1 - timeout error
#
#  SEE ALSO
#     sge_procedures/add_pe()
#*******************************
proc del_pe { mype_name } {
   
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_CORE_MASTER CHECK_USER CHECK_HOST
  global CHECK_OUTPUT

  if { [get_pe_ckpt_version] > 0 } {
     puts $CHECK_OUTPUT "searching for references in all defined queues ..."
     set NO_QUEUE_DEFINED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "queue"]
     catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-sql" } result
     if { [string first $NO_QUEUE_DEFINED $result] >= 0 } {
        puts $CHECK_OUTPUT "no queue defined"
     } else {
        foreach elem $result {
           puts $CHECK_OUTPUT "queue: $elem"
           if {[info exists params]} {
              unset params
           }
           get_queue $elem params
           if { [string first $mype_name $params(pe_list)] >= 0 } {
              puts $CHECK_OUTPUT "pe obj $mype_name is referenced in queue $elem, removing entry."
              set new_params ""
              set help_list [split $params(pe_list) ","]
              set help_list2 ""
              foreach element $help_list {
                 append help_list2 "$element "
              }
              set help_list [string trim $help_list2] 
              foreach help_pe $help_list {
                 if { [string match $mype_name $help_pe] != 1 } {
                    append new_params "$help_pe "
                 }
              }
              if { [llength $new_params ] == 0 } {
                 set new_params "NONE"
              }
              set mod_params(pe_list) [string trim $new_params]
              set_queue $elem mod_params 
           }
        }
     }
  }

  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $mype_name "*" ]

  log_user 0 
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dp $mype_name"]

  set sp_id [ lindex $id 1 ]

  set result -1
  set timeout 30 	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_pe" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id $REMOVED {
      set result 0
    }
    -i $sp_id "removed" {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
   
  }
  close_spawn_process $id
  log_user 1
  if { $result != 0 } {
     add_proc_error "del_pe" -1 "could not delete pe \"$mype_name\""
  }
  return $result

}

#                                                             max. column:     |
#****** sge_procedures/del_prj() ******
# 
#  NAME
#     del_prj -- ??? 
#
#  SYNOPSIS
#     del_prj { myprj_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     myprj_name - ??? 
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
proc del_prj { myprj_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_PRODUCT_TYPE CHECK_USER CHECK_CORE_MASTER
  global CHECK_HOST

  if { [ string compare $CHECK_PRODUCT_TYPE "sge" ] == 0 } {
     set_error -1 "del_prj - not possible for sge systems"
     return
  }

  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $myprj_name "*" ]

  log_user 0
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dprj $myprj_name"]
  set sp_id [ lindex $id 1 ]
  set result -1
  set timeout 30 	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_prj" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id "removed" {
      set result 0
    }
    -i $sp_id $REMOVED {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
  }
  close_spawn_process $id
  log_user 1
  if { $result != 0 } {
     add_proc_error "del_prj" -1 "could not delete project \"$myprj_name\""
  }
  return $result
}

#                                                             max. column:     |
#****** sge_procedures/del_user() ******
# 
#  NAME
#     del_user -- ??? 
#
#  SYNOPSIS
#     del_user { myuser_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     myuser_name - ??? 
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
proc del_user { myuser_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_PRODUCT_TYPE CHECK_HOST CHECK_USER CHECK_OUTPUT

  if { [ string compare $CHECK_PRODUCT_TYPE "sge" ] == 0 } {
     set_error -1 "del_user - not possible for sge systems"
     return
  }

  set REMOVED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" "*" "*" ]

  log_user 0
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-duser $myuser_name"]
   

  set sp_id [ lindex $id 1 ]
  set result -1
  set timeout 30 	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_user" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id "removed" {
      set result 0
    }
    -i $sp_id $REMOVED {
      set result 0
    }
    -i $sp_id default {
      set result -1
    }
  }
  close_spawn_process $id
  log_user 1
  if { $result != 0 } {
     add_proc_error "del_user" -1 "could not delete user \"$myuser_name\""
  } else {
     puts $CHECK_OUTPUT "removed user \"$myuser_name\""
  }
  return $result
}

#                                                             max. column:     |
#****** sge_procedures/del_calendar() ******
# 
#  NAME
#     del_calendar -- ??? 
#
#  SYNOPSIS
#     del_calendar { mycal_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     mycal_name - ??? 
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
proc del_calendar { mycal_name } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_CORE_MASTER CHECK_USER CHECK_HOST
  
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $mycal_name "*" ]

  log_user 0
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dcal $mycal_name"]

  set sp_id [ lindex $id 1 ]
  set timeout 30 
  set result -1	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_calendar" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id "removed" {
      set result 0
    }
    -i $sp_id $REMOVED {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
    
  }
  close_spawn_process $id
  log_user 1

  if { $result != 0 } {
     add_proc_error "del_calendar" -1 "could not delete calendar \"$mycal_name\""
  }

  return $result
}




#                                                             max. column:     |
#****** sge_procedures/add_calendar() ******
# 
#  NAME
#     add_calendar -- add new calendar definition object
#
#  SYNOPSIS
#     add_calendar { change_array } 
#
#  FUNCTION
#     This procedure will add/define a new calendar definition object
#
#  INPUTS
#     change_array - name of an array variable that will be set by add_calendar
#
#  RESULT
#     -1   timeout error
#     -2   callendar allready exists
#      0   ok
#
#  EXAMPLE
#     set new_cal(calendar_name)  "always_suspend"
#     set new_cal(year)           "NONE"
#     set new_cal(week)           "mon-sun=0-24=suspended" 
#
#  NOTES
#     The array should look like this:
#
#     set change_array(calendar_name) "mycalendar"
#     set change_array(year) 	        "NONE"
#     set change-array(week)          "mon-sun=0-24=suspended"
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#
#     attribute(calendar_name) "test"
#     attribute(year)          "NONE"
#     attribute(week)          "NONE"
#
#  SEE ALSO
#     ???/???
#*******************************
proc add_calendar { change_array } {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

  upvar $change_array chgar

  puts $CHECK_OUTPUT "adding calendar $chgar(calendar_name)"
  set values [array names chgar]

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal [set chgar($elem)]
     set newVal1 [split $newVal {/}]
     set newVal [join $newVal1 {\/}]
     lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
  } 
  # xyz is neccessary, as there is no template for calendards existing!!!
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-acal xyz" $vi_commands $ADDED $ALREADY_EXISTS]
  if { $result == -1 } { add_proc_error "add_calendar" -1 "timeout error" }
  if { $result == -2 } { add_proc_error "add_calendar" -1 "\"[set chgar(calendar_name)]\" already exists" }
  if { $result != 0  } { add_proc_error "add_calendar" -1 "could not add callendar \"[set chgar(calendar_name)]\"" }
  return $result
}


#                                                             max. column:     |
#****** sge_procedures/was_job_running() ******
# 
#  NAME
#     was_job_running -- look for job accounting
#
#  SYNOPSIS
#     was_job_running { jobid {do_errorcheck 1} } 
#
#  FUNCTION
#     This procedure will start a qacct -j jobid. If the hob was not found in
#     the output of the qacct command, this function will return -1. This
#     means that the job is still running, or was never running.
#
#  INPUTS
#     jobid             - job identification number
#     {do_errorcheck 1} - 1: call add_proc_error if job was not found
#                         0: do not generate error messages
#
#  RESULT
#     "-1"  : if job was not found
#     or the output of qacct -j
#
#  SEE ALSO
#     check/add_proc_error()
#*******************************
proc was_job_running {jobid {do_errorcheck 1} } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer check_timestamp CHECK_OUTPUT
# returns
# -1 on error
# qacct in listform else

  set mytime [timestamp]

  if { $mytime == $check_timestamp } {
     puts $CHECK_OUTPUT "was_job_running - waiting for job ..."
     sleep 1
  }
  set check_timestamp $mytime

  set catch_state [ catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qacct" "-j" "$jobid" } result ]
  
  if { $catch_state != 0 } {
      if {$do_errorcheck == 1 } { 
            puts $CHECK_OUTPUT "debug: $result"
            add_proc_error "was_job_running" -1 "job \"($jobid)\" not found"
      }
      return -1
  }

  set return_value [lreplace $result 0 0]
  return $return_value

}

#                                                             max. column:     |
#****** sge_procedures/slave_queue_of() ******
# 
#  NAME
#     slave_queue_of -- Get the last slave queue of a parallel job
#
#  SYNOPSIS
#     slave_queue_of { job_id } 
#
#  FUNCTION
#     This procedure will return the name of the last slave queue of a
#     parallel job or "" if the SLAVE queue was not found.
#
#  INPUTS
#     job_id - Identification number of the job
#
#  RESULT
#     empty or the last queue name on which the SLAVE task is running 
#
#  SEE ALSO
#     sge_procedures/master_queue_of()
#*******************************
proc slave_queue_of { job_id } {
   global CHECK_OUTPUT
# return last slave queue of job
# no slave -> return ""
   puts $CHECK_OUTPUT "Looking for SLAVE QUEUE of Job $job_id."
   set slave_queue ""         ;# return -1, if there is no slave queue
   
   set result [get_standard_job_info $job_id]
   foreach elem $result {
       set whatami [lindex $elem 8]
       if { [string compare $whatami "SLAVE"] == 0 } { 
          set slave_queue [lindex $elem 7]
          puts $CHECK_OUTPUT "Slave is running on queue \"$slave_queue\""
       }
   }
   if {[string compare $slave_queue ""] == 0 } {
     add_proc_error "slave_queue_of" -1 "no slave queue for job $job_id found"
   } 
   return $slave_queue
}

#                                                             max. column:     |
#****** sge_procedures/master_queue_of() ******
# 
#  NAME
#     master_queue_of -- get the master queue of a parallel job
#
#  SYNOPSIS
#     master_queue_of { job_id } 
#
#  FUNCTION
#     This procedure will return the name of the master queue of a
#     parallel job or "" if the MASTER queue was not found.
#
#  INPUTS
#     job_id - Identification number of the job
#
#  RESULT
#     empty or the last queue name on which the MASTER task is running 
#
#  SEE ALSO
#     sge_procedures/slave_queue_of()
#*******************************
proc master_queue_of { job_id } {
   global CHECK_OUTPUT
# return master queue of job
# no master -> return "": ! THIS MAY NOT HAPPEN !
   puts $CHECK_OUTPUT "Looking for MASTER QUEUE of Job $job_id."
   set master_queue ""         ;# return -1, if there is no slave queue
   
   set result [get_standard_job_info $job_id]
   foreach elem $result {
       set whatami [lindex $elem 8]
       if { [string compare $whatami "MASTER"] == 0 } { 
          set master_queue [lindex $elem 7]
          puts $CHECK_OUTPUT "Master is running on queue \"$master_queue\""
          break                  ;# break out of loop, if Master found
       }
   }
   if {[string compare $master_queue ""] == 0 } {
     add_proc_error "master_queue_of" -1 "no master queue for job $job_id found"
   }  
   return $master_queue
}


#                                                             max. column:     |
#****** sge_procedures/wait_for_load_from_all_queues() ******
# 
#  NAME
#     wait_for_load_from_all_queues -- wait for load value reports from queues
#
#  SYNOPSIS
#     wait_for_load_from_all_queues { seconds } 
#
#  FUNCTION
#     This procedure waits until all queues are reporting a load value smaller
#     than 99. If this is the case all execd should be successfully connected 
#     to the qmaster.
#
#  INPUTS
#     seconds - timeout value in seconds
#
#  RESULT
#     "-1" on error
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     sge_procedures/wait_for_unknown_load()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_load_from_all_queues { seconds } {
   global check_errno check_errstr CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_CORE_EXECD CHECK_HOST CHECK_OUTPUT

   set time [timestamp]

   while { 1 } {
      puts $CHECK_OUTPUT "waiting for load value report from all queues ..."
      sleep 1
      set result ""
      set catch_return [ catch {exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-f"} result ]
      if { $catch_return == 0 } {
         # split each line as listelement
         set help [split $result "\n"]
       
         #remove first line
         set help [lreplace $help 0 0]
         set data ""
       
         #get every line after "----..." line 
         set len [llength $help]
         for {set ind 0} {$ind < $len } {incr ind 1} {
            if {[lsearch [lindex $help $ind] "------*"] >= 0 } {
               lappend data [lindex $help [expr ( $ind + 1 )]] 
            }
         }
      
         set qcount [ llength $data]
         set qnames ""
         set slots ""
         set load ""
      
         # get line data information for queuename used/tot and load_avg
         foreach elem $data {
            set linedata $elem
            lappend qnames [lindex $linedata 0]
            set used_tot [lindex $linedata 2]
            set pos1 [ expr ( [string first "/" $used_tot] + 1 ) ]
            set pos2 [ expr ( [string length $used_tot]          - 1 ) ]
      
            lappend slots [string range $used_tot $pos1 $pos2 ]
            lappend load [lindex $linedata 3]
         }
      
         # check if load of an host is set > 99 (no exed report)
         set failed 0
         foreach elem $load {
           if {$elem >= 99} {
              incr failed 1
           }
         }
    
         if { $failed == 0 } {
            return 
         }
      } else {
        puts $CHECK_OUTPUT "qstat error or binary not found"
      }

      set runtime [expr ( [timestamp] - $time) ]
      if { $runtime >= $seconds } {
          add_proc_error "wait_for_load_from_all_queues" -1 "timeout waiting for load values < 99"
          return -1
      }
   }
}


#****** sge_procedures/wait_for_job_state() ************************************
#  NAME
#     wait_for_job_state() -- wait for job to become special job state
#
#  SYNOPSIS
#     wait_for_job_state { jobid state wait_timeout } 
#
#  FUNCTION
#     This procedure is checking the job state of the given job id by parsing
#     the qstat -f command. If the job has the state given in the parameter
#     state the procedure returns the job state. If an timeout occurs the 
#     procedure returns -1.
#
#  INPUTS
#     jobid        - job id of job to check state
#     state        - state to check for
#     wait_timeout - given timeout in seconds
#
#  RESULT
#     job state or -1 on error
#
#  SEE ALSO
#     sge_procedures/wait_for_queue_state()
#*******************************************************************************
proc wait_for_job_state { jobid state wait_timeout } {

   global CHECK_OUTPUT 

   set my_timeout [ expr ( [timestamp] + $wait_timeout ) ]
   while { 1 } {
      puts $CHECK_OUTPUT "waiting for job $jobid to become job state ${state} ..."
      sleep 1
      set job_state [get_job_state $jobid]
      if { [string first $state $job_state] >= 0 } {
         return $job_state
      }
      if { [timestamp] > $my_timeout } {
         add_proc_error "wait_for_job_state" -1 "timeout waiting for job $jobid to get in \"$state\" state"
         return -1
      }
   }
}

#****** sge_procedures/wait_for_queue_state() **********************************
#  NAME
#     wait_for_queue_state() -- wait for queue to become special error state
#
#  SYNOPSIS
#     wait_for_queue_state { queue state wait_timeout } 
#
#  FUNCTION
#     This procedure is checking the queue by parsing the qstat -f command. 
#
#  INPUTS
#     queue        - name of queue to check
#     state        - state to check for
#     wait_timeout - given timeout in seconds
#
#  RESULT
#     queue state or -1 on error
#
#  SEE ALSO
#     sge_procedures/wait_for_job_state()
#*******************************************************************************
proc wait_for_queue_state { queue state wait_timeout } {
   global CHECK_OUTPUT

   set my_timeout [ expr ( [timestamp] + $wait_timeout ) ]
   while { 1 } {
      puts $CHECK_OUTPUT "waiting for queue $queue to get in \"${state}\" state ..."
      sleep 1
      set q_state [get_queue_state $queue]
      if { [string first $state $q_state] >= 0 } {
         return $q_state
      }
      if { [timestamp] > $my_timeout } {
         add_proc_error "wait_for_queue_state" -1 "timeout waiting for queue $queue to get in \"${state}\" state"
         return -1
      }
   }
}


#****** sge_procedures/soft_execd_shutdown() ***********************************
#  NAME
#     soft_execd_shutdown() -- soft shutdown of execd
#
#  SYNOPSIS
#     soft_execd_shutdown { host } 
#
#  FUNCTION
#     This procedure starts a qconf -ke $host. If qconf reports an error,
#     the execd is killed by the shtudown_system_deamon() procedure.
#     After that qstat is called. When the load value for the given 
#     host-queue ($host.q) is 99.99 the procedure returns without error.
#     
#  INPUTS
#     host - execution daemon host to shutdown
#
#  RESULT
#     0 - success
#    -1 - error
#
#  SEE ALSO
#     sge_procedures/shutdown_system_daemon()
#*******************************************************************************
proc soft_execd_shutdown { host } {
 
   global CHECK_CORE_MASTER CHECK_OUTPUT
   global CHECK_PRODUCT_ROOT CHECK_USER CHECK_ARCH

   set tries 0
   while { $tries <= 8 } {   
      incr tries 1
      set result [ start_remote_prog $CHECK_CORE_MASTER $CHECK_USER $CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf "-ke $host" ]
      if { $prg_exit_state != 0 } {
         puts $CHECK_OUTPUT "qconf -ke $host returned $prg_exit_state, hard killing execd"
         shutdown_system_daemon $host execd
      }
      set load [wait_for_unknown_load 15 "${host}.q" 0]
      if { $load == 0 } {
         puts $CHECK_OUTPUT "execd on host $host reports 99.99 load value"
         return 0
      }
   }
   add_proc_error "soft_execd_shutdown" -1 "could not shutdown execd on host $host"
   return -1
}

#****** sge_procedures/wait_for_unknown_load() *********************************
#  NAME
#     wait_for_unknown_load() -- wait for load to get >= 99 for a list of queues
#
#  SYNOPSIS
#     wait_for_unknown_load { seconds queue_array { do_error_check 1 } } 
#
#  FUNCTION
#     This procedure is starting the qstat -f command and parse the output for
#     the queue load values. If the load value of the given queue(s) have a value
#     greater than 99 the procedure will return. If not an error message is
#     generated after timeout.
#
#  INPUTS
#     seconds        - number of seconds to wait before creating timeout error
#     queue_array    - an array of queue names for which to wait
#     do_error_check - (optional) if 1: report errors
#                                 if 0: don't report errors
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#  
#*******************************************************************************
proc wait_for_unknown_load { seconds queue_array { do_error_check 1 } } {
   global check_errno check_errstr CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_CORE_EXECD CHECK_HOST CHECK_OUTPUT

   set time [timestamp]
   while { 1 } {
      sleep 1
      puts $CHECK_OUTPUT "wait_for_unknown_load - waiting for queues\n\"$queue_array\"\nto get unknown load state ..."
      set result ""
      set catch_return [ catch {exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-f"} result ]
      if { $catch_return == 0 } {
         # split each line as listelement
         set help [split $result "\n"]
       
         #remove first line
         set help [lreplace $help 0 0]
         set data ""
       
         #get every line after "----..." line 
         set len [llength $help]
         for {set ind 0} {$ind < $len } {incr ind 1} {
            if {[lsearch [lindex $help $ind] "------*"] >= 0 } {
               lappend data [lindex $help [expr ( $ind + 1 )]] 
            }
         }
      
         set qcount [ llength $data]
         set qnames ""
         set slots ""
         set load ""
      
         # get line data information for queuename used/tot and load_avg
         foreach elem $data {
            set linedata $elem

            set queue_name [lindex $linedata 0]
            set load_value [lindex $linedata 3]
            set load_values($queue_name) $load_value
         }
      
         # check if load of an host is set > 99 (no exed report)
         set failed 0
         
         foreach queue $queue_array {
            if { [info exists load_values($queue)] == 1 } {
               if { $load_values($queue) < 99 } {
                   incr failed 1
               } 
            }
         }

         if { $failed == 0 } {
            return 0
         }
      } else {
        puts $CHECK_OUTPUT "qstat error or binary not found"
      }

      set runtime [expr ( [timestamp] - $time) ]
      if { $runtime >= $seconds && $do_error_check == 1 } {
          add_proc_error "wait_for_unknown_load" -1 "timeout waiting for load values >= 99"
          return -1
      }
   }
   return 0
}


#
#                                                             max. column:     |
#
#****** sge_procedures/wait_for_end_of_all_jobs() ******
#  NAME
#     wait_for_end_of_all_jobs() -- wait for end of all jobs
#
#  SYNOPSIS
#     wait_for_end_of_all_jobs { seconds } 
#
#  FUNCTION
#     This procedure will wait until no further jobs are remaining in the cluster.
#
#  INPUTS
#     seconds - timeout value (if < 1 no timeout is set)
#
#  RESULT
#     0 - ok
#    -1 - timeout
#
#  SEE ALSO
#     sge_procedures/wait_for_jobend()
#*******************************
#
proc wait_for_end_of_all_jobs { seconds } {
   global check_errno check_errstr CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_CORE_EXECD CHECK_HOST CHECK_OUTPUT

   set time [timestamp]

   while { 1 } {
      puts $CHECK_OUTPUT "waiting for end of all jobs ..."
      sleep 1
      set result ""
      set catch_return [ catch {eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat -s pr" } result ]
      if { $catch_return == 0 } {
         if { [ string compare $result "" ] == 0 } {
            puts $CHECK_OUTPUT "no further jobs in system"
            return 0
         }

         # split each line as listelement
         set help [split $result "\n"]
 
         #remove first two lines
         set help [lreplace $help 0 1]
         
         foreach elem $help {
            puts $CHECK_OUTPUT $elem
         }
      } else {
        puts $CHECK_OUTPUT "qstat error or binary not found"
      }

      if { $seconds > 0 } {
         set runtime [expr ( [timestamp] - $time) ]
         if { $runtime >= $seconds } {
             add_proc_error "wait_for_end_of_all_jobs" -1 "timeout waiting for end of all jobs:\n\"$result\""
             return -1
         }
      }
   }
}

#                                                             max. column:     |
#****** sge_procedures/mqattr() ******
# 
#  NAME
#     mqattr -- Modify queue attributes
#
#  SYNOPSIS
#     mqattr { attribute entry queue_list } 
#
#  FUNCTION
#     This procedure enables the caller to modify particular queue attributes.
#     Look at set_queue for queue attributes.
#
#  INPUTS
#     attribute  - name of attribute to modify
#     entry      - new value for attribute
#     queue_list - name of queues to change
#
#  RESULT
#     -1 - error
#     0  - ok
#
#  EXAMPLE
#     set return_value [mqattr "calendar" "always_disabled" "$queue_list"]
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc mqattr { attribute entry queue_list } {
# returns
# -1 on error
# 0 on success
  
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_CORE_MASTER CHECK_OUTPUT CHECK_USER

  puts "Trying to change attribute $attribute of queues $queue_list to $entry."

  set help "$attribute \"$entry\""   ;# create e.g. slots "5" as string
  set catch_return [ catch {  
    eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -mqattr $help $queue_list" 
  } result ]
  if { $catch_return != 0 } {
     add_proc_error "mqattr" "-1" "qconf error or binary not found"
     return -1
  }

  # split each line as listelement
  set help [split $result "\n"]
  set counter 0
  set return_value 0
  
  foreach elem $help {
     set queue_name [lindex $queue_list $counter]
     set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $queue_name "*"]
     if { [ string match "*$MODIFIED*" $elem ] != 1 } {
        puts $CHECK_OUTPUT "Could not modify queue $queue_name."
        set return_value -1
     } else {
        puts "Modified queue $queue_name successfully."
     }
     incr counter 1
   }                                       

   if { $return_value != 0 } {
      add_proc_error "mqattr" -1 "could not modify queue \"$queue_name\""
   }

   return $return_value
}





#                                                             max. column:     |
#****** sge_procedures/suspend_job() ******
# 
#  NAME
#     suspend_job -- set job in suspend state
#
#  SYNOPSIS
#     suspend_job { id } 
#
#  FUNCTION
#     This procedure will call qmod to suspend the given job id.
#
#  INPUTS
#     id - job identification number
#
#  RESULT
#     0  - ok
#     -1 - error
#
#  SEE ALSO
#     sge_procedures/unsuspend_job()
#*******************************
proc suspend_job { id } {
   global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_USER CHECK_HOST

   set SUSPEND1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUSPENDTASK_SUU] "*" "*" "*" ]
   set SUSPEND2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUSPENDJOB_SU] "*" "*" ]


   log_user 0 
	set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod"
   set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-s $id"  ]     

   set sp_id [ lindex $sid 1 ]
	set timeout 30
   set result -1	
   log_user 0 

	expect {
      -i $sp_id full_buffer {
         set result -1 
         add_proc_error "suspend_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
      }
	   -i $sp_id $SUSPEND1 {
	      set result 0 
	   }
	   -i $sp_id $SUSPEND2 {
	      set result 0 
	   }
	   -i $sp_id "suspended job" {
	      set result 0 
	   }
      -i $sp_id default {
	      set result -1 
	   }
	}
	# close spawned process 
	close_spawn_process $sid
   log_user 1
   if { $result != 0 } {
      add_proc_error "suspend_job" -1 "could not suspend job $id"
   }
   return $result
}

#                                                             max. column:     |
#****** sge_procedures/unsuspend_job() ******
# 
#  NAME
#     unsuspend_job -- set job bakr from unsuspended state
 
#
#  SYNOPSIS
#     unsuspend_job { job } 
#
#  FUNCTION
#     This procedure will call qmod to unsuspend the given job id.
#
#  INPUTS
#     job - job identification number

#
#  RESULT
#     0   - ok
#     -1  - error
#
#  SEE ALSO
#     sge_procedures/suspend_job()
#*******************************
proc unsuspend_job { job } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_HOST CHECK_USER


  set UNSUSPEND1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_UNSUSPENDTASK_SUU] "*" "*" "*" ]
  set UNSUSPEND2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_UNSUSPENDJOB_SU] "*" "*" ]

  log_user 0 
  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod"
  set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-us $job" ]
  set sp_id [ lindex $sid 1 ]
  set timeout 30
  set result -1	
  log_user 0 

  expect {
       -i $sp_id full_buffer {
          set result -1 
          add_proc_error "unsuspend_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
       }
       -i $sp_id $UNSUSPEND1 {
          set result 0 
       }
       -i $sp_id $UNSUSPEND2 {
          set result 0 
       }
       -i $sp_id "unsuspended job" {
          set result 0 
       }
       -i $sp_id default {
          set result -1 
       }
       
  }

  # close spawned process 
  close_spawn_process $sid
  log_user 1   
  if { $result != 0 } {
     add_proc_error "unsuspend_job" -1 "could not unsuspend job $job"
  }
  return $result
}


#****** sge_procedures/is_job_id() *********************************************
#  NAME
#     is_job_id() -- check if job_id is a real sge job id
#
#  SYNOPSIS
#     is_job_id { job_id } 
#
#  FUNCTION
#     This procedure returns 1 if the given job id is a guilty sge job id
#
#  INPUTS
#     job_id - job id
#
#  RESULT
#     1 on success, 0 on error
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc is_job_id { job_id } {

   if { [string is integer $job_id] != 1} {
      if { [set pos [string first "." $job_id ]] >= 0 } {
         incr pos -1
         set array_id [  string range $job_id 0 $pos] 
         incr pos 2
         set task_rest [ string range $job_id $pos end]
         if { [string is integer $array_id] != 1} {
            add_proc_error "is_job_id" -1 "unexpected task job id: $array_id (no integer)"
            return 0
         } 
      } else {
         add_proc_error "is_job_id" -1 "unexpected job id: $job_id (no integer)"
         return 0
      }
   }
   if { $job_id <= 0 } {
      add_proc_error "is_job_id" -1 "unexpected job id: $job_id (no positive number)"
      return 0
   }
   return 1
}

#                                                             max. column:     |
#****** sge_procedures/delete_job() ******
# 
#  NAME
#     delete_job -- delete job with jobid
#
#  SYNOPSIS
#     delete_job { jobid { wait_for_end 0 }} 
#
#  FUNCTION
#     This procedure will delete the job with the given jobid
#
#  INPUTS
#     jobid              - job identification number
#     { wait_for_end 0 } - optional, if not 0: wait for end of job 
#                          (till qstat -f $jobid returns "job not found")
#
#  RESULT
#     0   - ok
#    -1   - timeout error
#
#  SEE ALSO
#     sge_procedures/submit_job()
#*******************************
proc delete_job { jobid { wait_for_end 0 }} {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer CHECK_HOST
   global CHECK_USER


#   sleep 1
   set REGISTERED1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_REGDELTASK_SUU] "*" "*" "*"]
   set REGISTERED2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_REGDELJOB_SU] "*" "*" ]
   set DELETED1  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_DELETETASK_SUU] "*" "*" "*"]
   set DELETED2  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_DELETEJOB_SU] "*" "*" ]

   set result -100
   if { [ is_job_id $jobid] } {
      # spawn process
      set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qdel"
      set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$program" "$jobid" ]
      set sp_id [ lindex $id 1 ]
      set timeout 60 	
      log_user 1
   
      while { $result == -100 } {
      expect {
          -i $sp_id full_buffer {
             set result -5
             add_proc_error "delete_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
          }
          -i $sp_id $REGISTERED1 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id $REGISTERED2 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id "registered the job" {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id  $DELETED1 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id  $DELETED2 {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i $sp_id "has deleted job" {
             puts $CHECK_OUTPUT $expect_out(0,string)
             set result 0
          }
          -i default {
             if { [info exists expect_out(buffer)] } {
                puts $CHECK_OUTPUT $expect_out(buffer)
                add_proc_error "delete_job" -1 "expect default switch\noutput was:\n>$expect_out(buffer)<"
             }
             set result -1 
          }
      }
      }
      # close spawned process 
      close_spawn_process $id 1
      log_user 1
   } else {
      add_proc_error "delete_job" -1 "job id is no integer"
   }
   if { $result != 0 } {
      add_proc_error "delete_job" -1 "could not delete job $jobid\nresult=$result"
   }
   if { $wait_for_end != 0 && $result == 0 } {
      set my_timeout [timestamp]
      set my_second_qdel_timeout $my_timeout
      incr my_second_qdel_timeout 80
      incr my_timeout 160
      while { [get_qstat_j_info $jobid ] != 0 } {
          puts $CHECK_OUTPUT "waiting for jobend ..."
          if { [timestamp] > $my_timeout } {
             add_proc_error "delete_job" -1 "timeout while waiting for jobend"
             break;
          }
          if { [timestamp] > $my_second_qdel_timeout } {
             set my_second_qdel_timeout $my_timeout
             delete_job $jobid
          }
          sleep 1
      }
   }
   return $result
}


#                                                             max. column:     |
#****** sge_procedures/submit_job() ******
# 
#  NAME
#     submit_job -- submit a job with qsub
#
#  SYNOPSIS
#     submit_job { args {do_error_check 1} {submit_timeout 60} {host ""} 
#                  {user ""} { cd_dir ""} { show_args 1 }  }
#
#  FUNCTION
#     This procedure will submit a job.
#
#  INPUTS
#     args                - a string of qsub arguments/parameters
#     {do_error_check 1}  - if 1 (default): add global erros (add_proc_error)
#                           if not 1: do not add errors
#     {submit_timeout 30} - timeout (default is 30 sec.)
#     {host ""}           - host on which to execute qsub (default $CHECK_HOST)
#     {user ""}           - user who shall submit job (default $CHECK_USER)
#     {cd_dir ""}         - optional: do cd to given directory first
#     { show_args 1 }     - optional: show job arguments
#
#  RESULT
#     This procedure returns:
#     
#     jobid   of array or job if submit was successfull (value > 1)
#        -1   on timeout error
#        -2   if usage was printed on -help or commandfile argument
#        -3   if usage was printed NOT on -help or commandfile argument
#        -4   if verify output was printed on -verify argument
#        -5   if verify output was NOT printed on -verfiy argument
#        -6   job could not be scheduled, try later
#        -7   has to much tasks - error
#        -8   unknown resource - error
#        -9   can't resolve hostname - error
#       -10   resource not requestable - error
#       -11   not allowed to submit jobs - error
#       -12   no access to project - error
#       -13   unkown option - error
#      -100   on error 
#     
#
#  EXAMPLE
#     set jobs ""
#     set my_outputs "-o /dev/null -e /dev/null"
#     set arguments "$my_outputs -q $rerun_queue -r y $CHECK_PRODUCT_ROOT/examples/jobs/sleeper.sh 1000"
#     lappend jobs [submit_job $arguments]
#
#  SEE ALSO
#     sge_procedures/delete_job()
#     check/add_proc_error()
#*******************************
proc submit_job { args {do_error_check 1} {submit_timeout 60} {host ""} {user ""} { cd_dir ""} { show_args 1 } } {
  global CHECK_PRODUCT_ROOT CHECK_HOST CHECK_ARCH CHECK_OUTPUT CHECK_USER
  global open_spawn_buffer CHECK_DEBUG_LEVEL

  set return_value " "

  if {$host == ""} {
    set host $CHECK_HOST
  }

  if {$user == ""} {
    set user $CHECK_USER
  }

  set arch [resolve_arch $host]

  set JOB_SUBMITTED       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOB_USS] "*" "*" "*"]
  set ERROR_OPENING       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_FILE_ERROROPENINGXY_SS] "*" "*"]
  set NOT_ALLOWED_WARNING [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_NOTINANYQ_S] "*" ]
  set JOB_SUBMITTED_DUMMY [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOB_USS] "__JOB_ID__" "__JOB_NAME__" "__JOB_ARG__"]
  set JOB_ARRAY_SUBMITTED       [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOBARRAY_UUUUSS] "*" "*" "*" "*" "*" "*" ]
  set JOB_ARRAY_SUBMITTED_DUMMY [translate $CHECK_HOST 1 0 0 [sge_macro MSG_JOB_SUBMITJOBARRAY_UUUUSS] "__JOB_ID__" "" "" "" "__JOB_NAME__" "__JOB_ARG__"]


  set TRY_LATER           [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER]]
  set SUCCESSFULLY        [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U] "*"]
  set USAGE               [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_USAGE_USAGESTRING] ]

  set UNAMBIGUOUSNESS [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_MOD_JOBNAMEVIOLATESJOBNET_SSUU] "*" "*" "*" "*" ]
  set NON_AMBIGUOUS   [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_MOD_JOBNETPREDECESSAMBIGUOUS_SUU] "*" "*" "*" ]
  set UNKNOWN_OPTION  [translate $CHECK_HOST 1 0 0   [sge_macro MSG_ANSWER_UNKOWNOPTIONX_S] "*" ]
  set NO_ACC_TO_PRJ1  [translate $CHECK_HOST 1 0 0   [sge_macro MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS] "*" "*"]
  set NO_ACC_TO_PRJ2  [translate $CHECK_HOST 1 0 0   [sge_macro MSG_STREE_USERTNOACCESS2PRJ_SS] "*" "*"]
  set NOT_ALLOWED1    [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_NOPERMS_SS] "*" "*"]
  set NOT_ALLOWED2    [translate $CHECK_HOST 1 0 0   [sge_macro MSG_JOB_PRJNOSUBMITPERMS_S] "*" ]
  set NOT_REQUESTABLE [translate $CHECK_HOST 1 0 0   [sge_macro MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S] "*" ]
  set CAN_T_RESOLVE   [translate $CHECK_HOST 1 0 0   [sge_macro MSG_SGETEXT_CANTRESOLVEHOST_S] "*" ]
  set UNKNOWN_RESOURCE1 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWN_RESOURCE_S] "*" ]
  set UNKNOWN_RESOURCE2 [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE] ]
  set TO_MUCH_TASKS [translate $CHECK_HOST 1 0 0     [sge_macro MSG_JOB_MORETASKSTHAN_U] "*" ]
  set WARNING_OPTION_ALREADY_SET [translate $CHECK_HOST 1 0 0 [sge_macro MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S] "*"]
  set ONLY_ONE_RANGE [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QCONF_ONLYONERANGE]]

  if { [resolve_version] >= 3 } {
     set COLON_NOT_ALLOWED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_COLONNOTALLOWED] ]
  } else {
     set help_translation  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_KEYSTR_COLON]]
     set COLON_NOT_ALLOWED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_GDI_KEYSTR_MIDCHAR_SC] "$help_translation" ":" ]
  }
   
  append USAGE " qsub"

  if { $show_args == 1 } {
     puts $CHECK_OUTPUT "job submit args:\n$args"
  }
  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$arch/qsub"
  if { $cd_dir != "" } {
     set id [ open_remote_spawn_process "$host" "$user" "cd" "$cd_dir;$program $args" ]
     puts $CHECK_OUTPUT "cd to $cd_dir"
  } else {
     set id [ open_remote_spawn_process "$host" "$user" "$program" "$args" ]
  }
  set sp_id [ lindex $id 1 ]

  set timeout $submit_timeout
  set do_again 1


  log_user 0  ;# debug log_user 0
  if { $CHECK_DEBUG_LEVEL != 0 } {
     log_user 1
  }

  while { $do_again == 1 } {
     set do_again 0
     expect {
          -i $sp_id full_buffer {
             set return_value -1    
             add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
          }
          -i $sp_id timeout {
             puts $CHECK_OUTPUT "submit_job - timeout(1)"
             add_proc_error "submit_job" "-1" "got timeout(1) error\nexpect_out(buffer):\n\"$expect_out(buffer)\""
             set return_value -1 
          }
          -i $sp_id eof {
             puts $CHECK_OUTPUT "submit_job - end of file unexpected"
             set return_value -1
          }
          -i $sp_id -- $NOT_ALLOWED_WARNING {
             puts $CHECK_OUTPUT "got warning: job can't run in any queue" 
             set outtext $expect_out(0,string) 
             puts $CHECK_OUTPUT "string is: \"$outtext\""
             set do_again 1
          }
          -i $sp_id -- $WARNING_OPTION_ALREADY_SET {
             puts $CHECK_OUTPUT "got warning: option has already been set" 
             set outtext $expect_out(0,string) 
             puts $CHECK_OUTPUT "string is: \"$outtext\""
             set do_again 1
          }
          -i $sp_id -- $JOB_SUBMITTED {
             set job_id_pos [ string first "__JOB_ID__" $JOB_SUBMITTED_DUMMY ]
             set job_name_pos [ string first "__JOB_NAME__" $JOB_SUBMITTED_DUMMY ]
             set job_arg_pos [ string first "__JOB_ARG__" $JOB_SUBMITTED_DUMMY ]
             if { $job_id_pos > $job_name_pos || $job_id_pos > $job_arg_pos } {
                add_proc_error "submit_job" "-1" "locale switches parameter for qsub string! This is not supported yet"
             }
             incr job_id_pos -1
             set job_id_prefix [ string range $JOB_SUBMITTED_DUMMY 0 $job_id_pos ]
             set job_id_prefix_length [ string length $job_id_prefix]
   
             set outtext $expect_out(0,string) 
#             puts $CHECK_OUTPUT "string is: \"$outtext\""
   #          puts $CHECK_OUTPUT "dummy  is: \"$JOB_SUBMITTED_DUMMY\""
             set id_pos [ string first $job_id_prefix $outtext]
             incr id_pos $job_id_prefix_length
             set submitjob_jobid [string range $outtext $id_pos end]
             set space_pos [ string first " " $submitjob_jobid ]
             set submitjob_jobid [string range $submitjob_jobid 0 $space_pos ]
             set submitjob_jobid [string trim $submitjob_jobid]
             if {[string first "." $submitjob_jobid] >= 0} {
                puts $CHECK_OUTPUT "This is a job array"
                set new_jobid [lindex [split $submitjob_jobid "."] 0]
                puts $CHECK_OUTPUT "Array has ID $new_jobid"
                set submitjob_jobid $new_jobid 
             }
   
   # try to figure out more
             set timeout 30
             expect {
                -i $sp_id full_buffer {
                   set return_value -1
                   add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                }
                -i $sp_id timeout {
                   puts $CHECK_OUTPUT "submit_job - timeout(2)"
                   set return_value -1 
                }
                -i $sp_id "_exit_status_" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id eof {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $SUCCESSFULLY  {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $TRY_LATER {
                   set return_value -6
                }
                
             }       
   
          }
          -i $sp_id -- $JOB_ARRAY_SUBMITTED {
             set job_id_pos [ string first "__JOB_ID__" $JOB_ARRAY_SUBMITTED_DUMMY ]
             set job_name_pos [ string first "__JOB_NAME__" $JOB_ARRAY_SUBMITTED_DUMMY ]
             set job_arg_pos [ string first "__JOB_ARG__" $JOB_ARRAY_SUBMITTED_DUMMY ]
             if { $job_id_pos > $job_name_pos || $job_id_pos > $job_arg_pos } {
                add_proc_error "submit_job" "-1" "locale switches parameter for qsub string! This is not supported yet"
             }
             incr job_id_pos -1
             set job_id_prefix [ string range $JOB_ARRAY_SUBMITTED_DUMMY 0 $job_id_pos ]
             set job_id_prefix_length [ string length $job_id_prefix]
   
             set outtext $expect_out(0,string) 
   #          puts $CHECK_OUTPUT "string is: \"$outtext\""
   #          puts $CHECK_OUTPUT "dummy  is: \"$JOB_ARRAY_SUBMITTED_DUMMY\""
             set id_pos [ string first $job_id_prefix $outtext]
             incr id_pos $job_id_prefix_length
             set submitjob_jobid [string range $outtext $id_pos end]
             set space_pos [ string first " " $submitjob_jobid ]
             set submitjob_jobid [string range $submitjob_jobid 0 $space_pos ]
             set submitjob_jobid [string trim $submitjob_jobid]
             if {[string first "." $submitjob_jobid] >= 0} {
                puts $CHECK_OUTPUT "This is a job array"
                set new_jobid [lindex [split $submitjob_jobid "."] 0]
                puts $CHECK_OUTPUT "Array has ID $new_jobid"
                set submitjob_jobid $new_jobid 
             }
   
   # try to figure out more
             set timeout 30
             expect {
                -i $sp_id full_buffer {
                   set return_value -1
                   add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                }
                -i $sp_id timeout {
                   puts $CHECK_OUTPUT "submit_job - timeout(2)"
                   set return_value -1 
                }
                -i $sp_id "_exit_status_" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id eof {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $SUCCESSFULLY  {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id $TRY_LATER {
                   set return_value -6
                }
                
             }       
   
          }
          
          
          -i $sp_id -- "job*has been submitted" {
             
             set outtext $expect_out(0,string) 
             set submitjob_jobid [lindex $outtext 1];
             if {[string first "." $submitjob_jobid] >= 0} {
                puts $CHECK_OUTPUT "This is a job array"
                set new_jobid [lindex [split $submitjob_jobid "."] 0]
                puts $CHECK_OUTPUT "Array has ID $new_jobid"
                set submitjob_jobid $new_jobid 
             }
   
   # try to figure out more
             set timeout 30
             expect {
                -i $sp_id full_buffer {
                   set return_value -1
                   add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
                }
                -i $sp_id timeout {
                   puts $CHECK_OUTPUT "submit_job - timeout(2)"
                   set return_value -1 
                }
                -i $sp_id "_exit_status_" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id eof {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id "successfully scheduled" {
                   puts $CHECK_OUTPUT "job submit returned ID: $submitjob_jobid"
                   set return_value $submitjob_jobid 
                }
                -i $sp_id "try again later" {
                   set return_value -6
                }
                
             }       
   
          }
   
          -i $sp_id -- "usage: qsub" {
             puts $CHECK_OUTPUT "got usage ..."
             if {([string first "help" $args] >= 0 ) || ([string first "commandfile" $args] >= 0)} {
                set return_value -2 
             } else { 
                set return_value -3
             }
          } 
          -i $sp_id -- $USAGE {
             puts $CHECK_OUTPUT "got usage ..."
             if {([string first "help" $args] >= 0 ) || ([string first "commandfile" $args] >= 0)} {
                set return_value -2 
             } else { 
                set return_value -3
             }
          } 
   
          -i $sp_id -- "job_number" {
            if {[string first "verify" $args] >= 0 } {
               set return_value -4
            } else {
               set return_value -5
            } 
          }
          -i $sp_id --  $TO_MUCH_TASKS {
             set return_value -7
          }
          -i $sp_id -- "submit a job with more than" {
             set return_value -7
          }
          -i $sp_id -- $UNKNOWN_RESOURCE1 {
             set return_value -8
          }
          -i $sp_id -- $UNKNOWN_RESOURCE2 {
             set return_value -8
          }
          -i $sp_id -- "unknown resource" {
             set return_value -8
          }
          -i $sp_id --  $CAN_T_RESOLVE {
             set return_value -9
          }
          -i $sp_id -- "can't resolve hostname" {
             set return_value -9
          }
          -i $sp_id --  $NOT_REQUESTABLE {
             set return_value -10
          }
          -i $sp_id -- "configured as non requestable" {
             set return_value -10
          }
          
          -i $sp_id --  $NOT_ALLOWED1 {
             set return_value -11
          }       
          -i $sp_id --  $NOT_ALLOWED2 {
             set return_value -11
          }       
          -i $sp_id -- "not allowed to submit jobs" {
             set return_value -11
          }
          -i $sp_id --  $NO_ACC_TO_PRJ1 {
             puts $CHECK_OUTPUT "got string(2): \"$expect_out(0,string)\""
             set return_value -12
          }
          -i $sp_id --  $NO_ACC_TO_PRJ2 {
         
             set return_value -12
          }
          -i $sp_id -- "no access to project" {
             set return_value -12
          }
          -i $sp_id -- $UNKNOWN_OPTION {
             set return_value -13
          }
          -i $sp_id -- "Unknown option" {
             set return_value -13
          }
          -i $sp_id -- $NON_AMBIGUOUS {
             set return_value -14
          }
          -i $sp_id -- "non-ambiguous jobnet predecessor" {
             set return_value -14
          }
          -i $sp_id -- $UNAMBIGUOUSNESS {
             set return_value -15
          }
          -i $sp_id -- "using job name \"*\" for*violates reference unambiguousness" {
             set return_value -15
          }
          -i $sp_id -- $ERROR_OPENING {
             set return_value -16
          }
          -i $sp_id -- $COLON_NOT_ALLOWED {
             set return_value -17
          }
          -i $sp_id -- $ONLY_ONE_RANGE {
             set return_value -18
          }
        }
     }
 
     # close spawned process 

     if { $do_error_check == 1 } {
        close_spawn_process $id
     } else {
        close_spawn_process $id 1
     }
   
     if {$do_error_check == 1} { 
       switch -- $return_value {
          "-1"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-2"  { add_proc_error "submit_job" 0  [get_submit_error $return_value]  }
          "-3"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-4"  { add_proc_error "submit_job" 0  [get_submit_error $return_value]  }
          "-5"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-6"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-7"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-8"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-9"  { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-10" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-11" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-12" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-13" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-14" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-15" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-16" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }
          "-17" { add_proc_error "submit_job" -1 [get_submit_error $return_value]  }

          default { add_proc_error "submit_job" 0 "job $return_value submitted - ok" }
       }
     }
     if { $return_value <= 0 && $do_error_check != 1 } {
        puts $CHECK_OUTPUT "submit_job returned error: [get_submit_error $return_value]"
     }
     puts $CHECK_OUTPUT "submit job: returning job id: $return_value"
     return $return_value
}



#****** sge_procedures/get_submit_error() **************************************
#  NAME
#     get_submit_error() -- resolve negative error value from submit_job()
#
#  SYNOPSIS
#     get_submit_error { error_id } 
#
#  FUNCTION
#     This procedure is used to get an error text from an negative return
#     value of the submit_job() procedure.
#
#  INPUTS
#     error_id - negative return value from submit_job() call
#
#  RESULT
#     Error text
#
#  SEE ALSO
#     sge_procedures/submit_job()
#*******************************************************************************
proc get_submit_error { error_id } {
   switch -- $error_id {
      "-1"  { return "timeout error" }
      "-2"  { return "usage was printed on -help or commandfile argument - ok" }
      "-3"  { return "usage was printed NOT on -help or commandfile argument - error" }
      "-4"  { return "verify output was printed on -verify argument - ok" }
      "-5"  { return "verify output was NOT printed on -verfiy argument - error" }
      "-6"  { return "job could not be scheduled, try later - error" }
      "-7"  { return "has to much tasks - error" }
      "-8"  { return "unknown resource - error" }
      "-9"  { return "can't resolve hostname - error" }
      "-10" { return "resource not requestable - error" }
      "-11" { return "not allowed to submit jobs - error" }
      "-12" { return "no acces to project - error" }
      "-13" { return "Unkown option - error" }
      "-14" { return "non-ambiguous jobnet predecessor - error" }
      "-15" { return "job violates reference unambiguousness - error" }
      "-16" { return "error opening file - error" }
      "-17" { return "colon not allowed in account string - error" }
      default { return "unknown error" }
   }
}

#                                                             max. column:     |
#****** sge_procedures/get_grppid_of_job() ******
# 
#  NAME
#     get_grppid_of_job -- get grppid of job
#
#  SYNOPSIS
#     get_grppid_of_job { jobid } 
#
#  FUNCTION
#     This procedure opens the job_pid file in the execution host spool directory
#     and returns the content of this file (grppid).
#     
#
#  INPUTS
#     jobid - identification number of job
#
#  RESULT
#     grppid of job 
#
#  SEE ALSO
#     sge_procedures/get_suspend_state_of_job()
#*******************************
proc get_grppid_of_job { jobid } {
   global CHECK_OUTPUT CHECK_HOST

   get_config value $CHECK_HOST
 
   if {[info exists value(execd_spool_dir)]} {
      set spool_dir $value(execd_spool_dir)
      puts $CHECK_OUTPUT "using local exec spool dir"  
   } else {
      puts $CHECK_OUTPUT "using global exec spool dir"  
      get_config value
      set spool_dir $value(execd_spool_dir)
   }

   puts $CHECK_OUTPUT "Exec Spool Dir is: $spool_dir"

   set pidfile "$spool_dir/$CHECK_HOST/active_jobs/$jobid.1/job_pid"

   sleep 5

   set back [ catch { open $pidfile "r" } fio ]

   set real_pid ""

   if { $back != 0  } {
      add_proc_error "get_grppid_of_job" -1 "can't open \"$pidfile\""
   } else {
      gets $fio real_pid
      close $fio
   }
   return $real_pid
}



#                                                             max. column:     |
#****** sge_procedures/get_suspend_state_of_job() ******
# 
#  NAME
#     get_suspend_state_of_job -- get suspend state of job from ps command
#
#  SYNOPSIS
#     get_suspend_state_of_job { jobid { pidlist pid_list } {do_error_check 1} 
#     } 
#
#  FUNCTION
#     This procedure returns the suspend state of jobid (letter from ps command). 
#     Beyond that a array (pidlist) is set, in which all process id of the process 
#     group are listed. The caller of the function can access the array pid_list!
#
#  INPUTS
#     jobid                - job identification number
#     { pidlist pid_list } - name of variable to store the pidlist
#     {do_error_check 1}   - enable error messages (add_proc_error), default
#                            if not 1 the procedure will not report errors
#
#  RESULT
#     suspend state (letter from ps command)
#
#  SEE ALSO
#     sge_procedures/get_grppid_of_job()
#     sge_procedures/add_proc_error()
#*******************************
proc get_suspend_state_of_job { jobid { pidlist pid_list } {do_error_check 1} } {
   global CHECK_OUTPUT CHECK_HOST CHECK_ARCH

   upvar $pidlist pidl 

   # give the system time to change the processes before ps call!!
   sleep 1

   # get process group id
   set real_pid [get_grppid_of_job $jobid]
   puts $CHECK_OUTPUT "grpid is \"$real_pid\" on host \"$CHECK_HOST\""


   set time_now [timestamp]
   set time_out [ expr ($time_now + 60 )  ]   ;# timeout is 60 seconds

   set have_errors 0
   while { [timestamp] < $time_out } {

      # get current process list (ps)
      get_ps_info $real_pid "local" ps_list
      
   
      # copy pid_list from ps_list
      set pscount $ps_list(proc_count)
      set pidl ""
      for {set i 0} { $i < $pscount } {incr i 1} {
         if { $ps_list(pgid,$i) == $real_pid } { 
            lappend pidl $ps_list(pid,$i)
         }
      } 
      puts $CHECK_OUTPUT "Process group has [llength $pidl] processes ($pidl)"
    
      #  
      set state_count 0
      set state_letter ""
      foreach elem $pidl {
         puts $CHECK_OUTPUT $ps_list($elem,string)
         if { $state_count == 0 } {
            set state_letter $ps_list($elem,state)
         }
         incr state_count 1
         if { ( [string compare $state_letter $ps_list($elem,state)] != 0 ) && ($do_error_check == 1 ) } {
            if { [string compare $state_letter "T"] == 0} { 
               set have_errors 1
               # we report an error if not all processes have the state "T
            }
         }
      }
      if { $have_errors == 0 } {
         break;
      }
   }

   if { $have_errors != 0 } {
      add_proc_error "get_suspend_state_of_job" -1 "not all processes in pgrp has the same state \"$state_letter\""
   }

   return $state_letter
}




#                                                             max. column:     |
#****** sge_procedures/get_job_info() ******
# 
#  NAME
#     get_job_info -- get qstat -ext jobinformation 
#
#  SYNOPSIS
#     get_job_info { jobid } 
#
#  FUNCTION
#     This procedure runs the qstat -ext command and returns the output
#
#  INPUTS
#     jobid - job id (if job id = -1 the complete joblist is returned)
#
#  RESULT
#     "" if job was not found or the call fails
#     output of qstat -ext
#
#  SEE ALSO
#     sge_procedures/get_job_info()
#     sge_procedures/get_standard_job_info()
#     sge_procedures/get_extended_job_info()
#*******************************
proc get_job_info { jobid } {
# return:
# info of qstat -ext for jobid
# nothing if job was not found
# complete joblist if jobid is -1
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_PRODUCT_TYPE

   if { [string compare $CHECK_PRODUCT_TYPE "sge"] == 0 } {
      add_proc_error "get_job_info" -1 "this call is not accepted for sge system"
      return "" 
   }

   set catch_return [ catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-ext" } result ]
   if { $catch_return != 0 } {
      add_proc_error "get_job_info" -1 "qstat error or binary not found"
      return ""
   }

  # split each line as listelement
   set back ""
   set help [split $result "\n"]
   foreach line $help { 
      if { [lindex $line 0] == $jobid } {
         set back $line
         return $back
      }
   }

   if { ($jobid == -1) && ( [ llength $help ] > 2 ) } {
      
      set help [lreplace $help 0 1]
      return $help
   }
   return $back
}

#                                                             max. column:     |
#****** sge_procedures/get_standard_job_info() ******
# 
#  NAME
#     get_standard_job_info -- get jobinfo with qstat
#
#  SYNOPSIS
#     get_standard_job_info { jobid { add_empty 0} { get_all 0 } } 
#
#  FUNCTION
#     This procedure will call the qstat command without arguments.
#
#  INPUTS
#     jobid           - job id 
#     { add_empty 0 } - if 1: add lines with does not contain a job id
#                       information (SLAVE jobs)
#     { get_all   0 } - if 1: get every output line (ignore job id)
#
#  RESULT
#     - info of qstat for jobid
#     - nothing if job was not found
#     
#     each list element has following sublists:
#     job-ID        (index 0)
#     prior         (index 1)
#     name          (index 2)
#     user          (index 3) 
#     state         (index 4) 
#     submit/start  (index 5)  
#     at            (index 6)
#     queue         (index 7)
#     master        (index 8)    
#     ja-task-ID    (index 9)     
#
#  EXAMPLE
#     set result [get_standard_job_info 5]
#     if { llength $results > 0 } {
#        puts "user [lindex $result 3] submitted job 5"
#     }
#
#
#  SEE ALSO
#     sge_procedures/get_job_info()
#     sge_procedures/get_standard_job_info()
#     sge_procedures/get_extended_job_info()
#*******************************
proc get_standard_job_info { jobid { add_empty 0} { get_all 0 } } {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT

   set catch_return [ catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" } result ]
   if { $catch_return != 0 } {
      add_proc_error "get_standard_job_info" -1 "qstat error or binary not found"
      return ""
   }


  # split each line as listelement
   set back ""
   set help [split $result "\n"]
   foreach line $help { 
#      puts $CHECK_OUTPUT $line
      if { [lindex $line 0] == $jobid } {
         lappend back $line
         continue
      }
      if { $add_empty != 0 } {
         if { [llength $line] == 8 } {
            lappend back "-1 $line"
            puts $CHECK_OUTPUT "adding empty job lines" 
            continue
         }
      }
      if { $get_all != 0 } {
            lappend back $line
      }
   }
   return $back
}

#                                                             max. column:     |
#****** sge_procedures/get_extended_job_info() ******
# 
#  NAME
#     get_extended_job_info -- get extended job information (qstat ..)
#
#  SYNOPSIS
#     get_extended_job_info { jobid {variable job_info} } 
#
#  FUNCTION
#     This procedure is calling the qstat (qstat -ext if sgeee) and returns
#     the output of the qstat in array form.
#
#  INPUTS
#     jobid               - job identifaction number
#     {variable job_info} - name of variable array to store the output
#
#  RESULT
#     0, if job was not found
#     1, if job was found
#     
#     fills array $variable with info found in qstat output with the following symbolic names:
#     id
#     prior
#     name
#     user
#     state
#     time (submit or starttime) [UNIX-timestamp]
#     queue
#     master
#     jatask
#    
#     additional entries in case of SGEEE system:
#     project
#     department
#     deadline [UNIX-timestamp]
#     cpu [s]
#     mem [GBs]
#     io [?]
#     tckts
#     ovrts
#     otckt
#     dtckt
#     ftckt
#     stckt
#     share
#
#  EXAMPLE
#  proc testproc ... { 
#     ...
#     if {[get_extended_job_info $job_id] } {
#        if { $job_info(cpu) < 10 } {
#           add_proc_error "testproc" -1 "online usage probably does not work on $host"
#        }
#     } else {
#        add_proc_error "testproc" -1 "get_extended_jobinfo failed for job $job_id on host $host"
#     }
#     ...
#     set_error 0 "ok"
#  }
#
#  SEE ALSO
#     sge_procedures/get_job_info()
#     sge_procedures/get_standard_job_info()
#     sge_procedures/get_extended_job_info()
#*******************************
proc get_extended_job_info {jobid {variable job_info}} {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_ARCH
   upvar $variable jobinfo

   if {$CHECK_PRODUCT_TYPE == "sgeee" } {
      set exit_code [catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" -ext} result]
      set ext 1
   } else {
      set exit_code [catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" } result]
      set ext 0
   }

   if { $exit_code == 0 } {
      parse_qstat result jobinfo $jobid $ext
      return 1
   }
  
   return 0
}

#****** sge_procedures/get_qstat_j_info() **************************************
#  NAME
#     get_qstat_j_info() -- get qstat -j information
#
#  SYNOPSIS
#     get_qstat_j_info { jobid {variable qstat_j_info} } 
#
#  FUNCTION
#     This procedure starts qstat -j for the given job id and returns
#     information in an tcl array. 
#
#  INPUTS
#     jobid                   - job id of job
#     {variable qstat_j_info} - array to store information
#
#  SEE ALSO
#     parser/parse_qstat_j()
#*******************************************************************************
proc get_qstat_j_info {jobid {variable qstat_j_info}} {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_ARCH
   global CHECK_OUTPUT
   upvar $variable jobinfo

   set exit_code [catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat -j $jobid" } result]
   if { $exit_code == 0 } {
      set result "$result\n"
      set my_result ""
      set help [split $result "\n"]
      foreach elem_org $help {
         set elem $elem_org
         # removing message id for localized strings
         if { [string first "\[" $elem ] == 0 } {
            set close_pos [ string first "\]" $elem ]
            incr close_pos 1
            set elem [ string range $elem $close_pos end]
            set elem [ string trim $elem]
            debug_puts "removing message id: \"$elem\""
         }
         if { [string first ":" $elem] >= 0 } {
            append my_result "\n$elem" 
         } else {
            append my_result ",$elem"
         }
      }
      set my_result "$my_result\n"
      parse_qstat_j my_result jobinfo $jobid 
      set a_names [array names jobinfo]
#      foreach elem $a_names {
#         puts "$elem: $jobinfo($elem)"
#      }
#      wait_for_enter
      return 1
   }
   return 0
}



#****** sge_procedures/get_qconf_se_info() *************************************
#  NAME
#     get_qconf_se_info() -- get qconf -se information
#
#  SYNOPSIS
#     get_qconf_se_info { hostname {variable qconf_se_info} } 
#
#  FUNCTION
#     This procedure starts qconf -se for the given hostname and returns
#     an tcl array with the output of the command.
#
#  INPUTS
#     hostname                 - execution host name
#     {variable qconf_se_info} - array to store information
#
#
#  SEE ALSO
#      parser/parse_qconf_se()
#*******************************************************************************
proc get_qconf_se_info {hostname {variable qconf_se_info}} {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_ARCH
   global CHECK_OUTPUT
   upvar $variable jobinfo

   set exit_code [catch { eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -se $hostname" } result]
   if { $exit_code == 0 } {
      set result "$result\n"
      parse_qconf_se result jobinfo $hostname 
      return 1
   }
   return 0
}




#                                                             max. column:     |
#****** sge_procedures/get_qacct() ******
# 
#  NAME
#     get_qacct -- get job accounting information
#
#  SYNOPSIS
#     get_qacct { jobid {variable qacct_info} } 
#
#  FUNCTION
#     This procedure will parse the qacct output for the given job id and fill 
#     up the given variable name with information.
#
#  INPUTS
#     jobid                 - job identification number
#     {variable qacct_info} - name of variable to save the results
#
#  RESULT
#     0, if job was not found
#     1, if job was found

#
#  EXAMPLE
#     
#     if { [get_qacct $job_id] == 0 } {
#        set_error -1 "qacct for job $job_id on host $host failed"
#     } else {
#        set cpu [expr $qacct_info(ru_utime) + $qacct_info(ru_stime)]
#        if { $cpu < 30 } {
#           set_error -1 "cpu entry in accounting ($qacct_info(cpu)) seems 
#                         to be wrong for job $job_id on host $host"
#        }
#
#        if { $CHECK_PRODUCT_TYPE == "sgeee" } {
#           # compute absolute diffence between cpu and ru_utime + ru_stime
#           set difference [expr $cpu - $qacct_info(cpu)]
#           set difference [expr $difference * $difference]
#           if { $difference > 1 } {
#              set_error -1 "accounting: cpu($qacct_info(cpu)) is not the 
#                            sum of ru_utime and ru_stime ($cpu) for 
#                            job $job_id on host $host"
#           }
#        }
#     }
#
#  NOTES
#     look at parser/parse_qacct for more information
#
#  SEE ALSO
#     parser/parse_qacct()
#*******************************
proc get_qacct {jobid {variable qacct_info}} {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT
   upvar $variable qacctinfo
   
   set exit_code [catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qacct" -j $jobid} result]
   if { $exit_code == 0 } {
      parse_qacct result qacctinfo $jobid
      return 1
   } else {
      puts $CHECK_OUTPUT "result of qacct -j $jobid:\n$result"
   }
   return 0
}


#                                                             max. column:     |
#****** sge_procedures/is_job_running() ******
# 
#  NAME
#     is_job_running -- get run information of job
#
#  SYNOPSIS
#     is_job_running { jobid jobname } 
#
#  FUNCTION
#     This procedure will call qstat -f for job information
#
#  INPUTS
#     jobid   - job identifaction number 
#     jobname - name of the job (string)
#
#  RESULT
#      0 - job is not running (but pending)
#      1 - job is running
#     -1 - not in stat list
#
#  NOTES
#     This procedure returns 1 (job is running) when the job
#     is spooled to a queue. This doesn not automatically mean
#     that the job is "real running".
#
#  SEE ALSO
#     sge_procedures/is_job_running()
#     sge_procedures/is_pid_with_name_existing()
#*******************************
proc is_job_running { jobid jobname } {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT check_timestamp

   set catch_state [ catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-f" } result ]

   set mytime [timestamp]
   if { $mytime == $check_timestamp } {
      sleep 1
   }
   set check_timestamp $mytime



   if { $catch_state != 0 } {
      puts $CHECK_OUTPUT "debug: $result"
      return -1
   }
#   puts $CHECK_OUTPUT "debug: catch_state: $catch_state"

   # split each line as listelement
   set help [split $result "\n"]
   set running_flag 1

   set found 0
   foreach line $help {
#     puts $CHECK_OUTPUT "debug: $line"
     if {[lsearch $line "####*"] >= 0} {
       set running_flag 0
     }

     if { ([string first $jobname $line ] >= 0) && ([lindex $line 0] == $jobid)  } {
       set found 1;
       break;
     }
   } 

   if { $found == 1 } {
      return $running_flag
   }
   return -1
}



#****** sge_procedures/get_job_state() *****************************************
#  NAME
#     get_job_state() -- get job state information
#
#  SYNOPSIS
#     get_job_state { jobid { not_all_equal 0 } { taskid task_id } } 
#
#  FUNCTION
#     This procedure parses the output of the qstat -f command and returns
#     the job state or an tcl array with detailed information
#
#  INPUTS
#     jobid               - Job id of job to get information for
#     { not_all_equal 0 } - if 0 (default): The procedure will wait until
#                           all tasks of a job array have the same state
#                           if 1: The procedure will return an tcl list
#                           with the job states and fill the array (given
#                           optional in parameter 3) "task_id" with information.
#     { taskid task_id }  - tcl array name to fill information if not_all_equal 
#                           is set to 1
#
#  RESULT
#     tcl array:
#          task_id($lfnr,state)     -> task state
#          task_id($lfnr,task)      -> task no
#
#          lfnr is a number between 0 and the length of the returned tcl list
#         
#
#*******************************************************************************
proc get_job_state { jobid { not_all_equal 0 } { taskid task_id } } {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT check_timestamp
   upvar $taskid r_task_id
   set mytime [timestamp]

   if { $mytime == $check_timestamp } {
      sleep 1
   }
   set check_timestamp $mytime

   set my_timeout [ expr ( $mytime + 100 ) ]
   set states_all_equal 0
   while { $my_timeout > [timestamp] && $states_all_equal == 0 } {   
      set states_all_equal 1

      set catch_state [ catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-f" } result ]
   
      if { $catch_state != 0 } {
         puts $CHECK_OUTPUT "debug: $result"
         return -1
      }
   #   puts $CHECK_OUTPUT "debug: catch_state: $catch_state"
   
      # split each line as listelement
      set help [split $result "\n"]
      set running_flag 1
   
      set states ""
      set lfnr 0
      foreach line $help {
        if { [lindex $line 0] == $jobid } {
           lappend states [lindex $line 4]
           debug_puts "debug: $line"
           if { [lindex $line 7] == "MASTER" } {
              set r_task_id($lfnr,task)  [lindex $line 8]
           } else {
              set r_task_id($lfnr,task)  [lindex $line 7]
           }
           set r_task_id($lfnr,state) [lindex $line 4]
           incr lfnr 1
        }
      }
      if { $states == "" } {
         set states -1
      }
      
      set main_state [lindex $states 0]
      if { $not_all_equal == 0 } {
         for { set elem 0 } { $elem < [llength $states] } { incr elem 1 } {
            if { [ string compare [lindex $states $elem] $main_state ] != 0 } {
               puts $CHECK_OUTPUT "jobstate of task $elem is: [lindex $states $elem], waiting ..."
               set states_all_equal 0
            } 
         }
         sleep 1
      }
   }
   if { $not_all_equal != 0 } {
      return $states
   }

   if { $states_all_equal == 1 } {
      return $main_state
   }
 
   add_proc_error "get_job_state" -1 "more than one job id found with different states"
   return -1
}

# wait for start of job ($jobid,$jobname) ; timeout after $seconds
# results : -1 on timeout ; 0 on jobstart
#                                                             max. column:     |
#****** sge_procedures/wait_for_jobstart() ******
# 
#  NAME
#     wait_for_jobstart -- wait for job to get out of pending list
#
#  SYNOPSIS
#     wait_for_jobstart { jobid jobname seconds {do_errorcheck 1} {do_tsm 0} } 
#
#  FUNCTION
#     This procedure will call the is_job_running procedure in a while
#     loop. When the job is scheduled to a queue the job is "running" 
#     and the procedure returns.
#
#  INPUTS
#     jobid             - job identification number
#     jobname           - name of the job
#     seconds           - timeout in seconds
#     {do_errorcheck 1} - enable error check (default)
#                         if 0: do not report errors
#     {do_tsm 0}        - do qconf -tsm before waiting
#                         if 1: do qconf -tsm (trigger scheduler) 
#
#  RESULT
#     -1 - job is not running (timeout error)
#      0 - job is running ( not in pending state)
#
#  EXAMPLE
#     foreach elem $jobs {
#        wait_for_jobstart $elem "Sleeper" 300
#        wait_for_end_of_transfer $elem 300
#        append jobs_string "$elem "
#     }
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_jobstart { jobid jobname seconds {do_errorcheck 1} {do_tsm 0} } {
  
  global CHECK_OUTPUT CHECK_PRODUCT_ROOT CHECK_ARCH

  if { [is_job_id $jobid] != 1  } {
     if { $do_errorcheck == 1 } {
          add_proc_error "wait_for_jobstart" -1 "got unexpected job id: $jobid"
     }
     return -1
  }

  if { $do_tsm == 1 } {
     puts $CHECK_OUTPUT "Trigger scheduler monitoring"
     catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-tsm" } result
     puts $CHECK_OUTPUT $result
  }

  puts $CHECK_OUTPUT "Waiting for start of job $jobid ($jobname)"
  if { $do_errorcheck != 1 } {
     puts $CHECK_OUTPUT "error check is switched off"
  }
   
  set time [timestamp]
  while {1} {
    
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == 1} {
       break;
    } 
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       if { $do_errorcheck == 1 } {
          add_proc_error "wait_for_jobstart" -1 "timeout waiting for job \"$jobid\" \"$jobname\""
       }
       return -1
    }
    sleep 1
  }
  return 0
}


#                                                             max. column:     |
#****** sge_procedures/wait_for_end_of_transfer() ******
# 
#  NAME
#     wait_for_end_of_transfer -- wait transfer end of job
#
#  SYNOPSIS
#     wait_for_end_of_transfer { jobid seconds } 
#
#  FUNCTION
#     This procedure will parse the qstat output of the job for the t state. If
#     no t state is found for the given job id, the procedure will return.
#
#  INPUTS
#     jobid   - job identification number
#     seconds - timeout in seconds
#
#  RESULT
#      0 - job is not in transferstate
#     -1 - timeout
#
#  EXAMPLE
#     see "sge_procedures/wait_for_jobstart"
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_end_of_transfer { jobid seconds } {
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for job $jobid to finish transfer state"
  
  set time [timestamp] 
  while {1} {
    set run_result [get_standard_job_info $jobid ]
    set job_state ""
    set had_error 0
    foreach line $run_result {
       set tmp_job_id [lindex $line 0]
       set tmp_job_state [ lindex $line 4 ]
       if { $tmp_job_id == $jobid } {
          if { $job_state == "" } {
             set job_state $tmp_job_state
          } else {
             if { $job_state != $tmp_job_state } {
                puts $CHECK_OUTPUT "job has different states ..."
                set had_error 1
                break
             }
          }
       }
    }

    if { $had_error != 0 } {
       sleep 1
       continue
    }

    if { [string first "t" $job_state ] < 0} {
       puts $CHECK_OUTPUT "job $jobid is running ($job_state)"
       break;
    }
    
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_end_of_transfer" -1 "timeout waiting for job \"$jobid\""
       return -1
    }
    sleep 1
  }
  return 0
}

# wait for job to be in pending state ($jobid,$jobname) ; timeout after $seconds
# results : -1 on timeout ; 0 on pending
#                                                             max. column:     |
#****** sge_procedures/wait_for_jobpending() ******
# 
#  NAME
#     wait_for_jobpending -- wait for job to get into pending state
#
#  SYNOPSIS
#     wait_for_jobpending { jobid jobname seconds { or_running 0 } } 
#
#  FUNCTION
#     This procedure will return when the job is in pending state.
#
#  INPUTS
#     jobid   - job identification number
#     jobname - name of the job
#     seconds - timeout value in seconds
#     { or_running 0 } - if job is allready running, report no error
#
#  RESULT
#     -1  on timeout
#     0   when job is in pending state
#
#  EXAMPLE
#     foreach elem $sched_jobs {
#         wait_for_jobpending $elem "Sleeper" 300
#     }
#
#  SEE ALSO
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_jobpending { jobid jobname seconds { or_running 0 } } {
  
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for job $jobid ($jobname) to get in pending state"
 

  if { [is_job_id $jobid] != 1} {
     puts $CHECK_OUTPUT "job is not integer"
     add_proc_error "wait_for_jobpending" -1 "unexpected job id: $jobid"
     return -1
  }

  set time [timestamp] 
  while {1} {
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == 0} {
       break;
    }
    if {$run_result == 1 && $or_running == 1  } {
       break;
    }
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_jobpending" -1 "timeout waiting for job \"$jobid\" \"$jobname\" (timeout was $seconds sec)"
       return -1
    }
    sleep 1
  }
  return 0
}


# set job in hold state
# results: -1 on timeout, 0 ok
#                                                             max. column:     |
#****** sge_procedures/hold_job() ******
# 
#  NAME
#     hold_job -- set job in hold state
#
#  SYNOPSIS
#     hold_job { jobid } 
#
#  FUNCTION
#     This procedure will use the qhold binary to set a job into hold state.
#
#  INPUTS
#     jobid - job identification number
#
#  RESULT
#        0 - ok
#       -1 - timeout error    
#
#  SEE ALSO
#     sge_procedures/release_job()
#     sge_procedures/hold_job()
#*******************************
proc hold_job { jobid } {

   global CHECK_PRODUCT_ROOT CHECK_ARCH  open_spawn_buffer CHECK_HOST CHECK_USER

   set MODIFIED_HOLD [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_MOD_JOBS_SU] "*" "*"]

   # spawn process
   log_user 0
   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qhold"
   set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "$jobid" ]

   set sp_id [ lindex $id 1 ]
   set timeout 30
   set result -1
   	
   log_user 0 

   expect {
       -i $sp_id full_buffer {
          set result -1 
          add_proc_error "hold_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value" 
       }
       -i $sp_id "modified hold of job" {
          set result 0
       }
       -i $sp_id $MODIFIED_HOLD {
          set result 0
       }
       -i $sp_id default {
          set result -1 
       }

   }
   # close spawned process 
   close_spawn_process $id
   log_user 1
   if { $result != 0 } {
      add_proc_error "hold_job" -1 "could not hold job $jobid"
   }
   return $result

}


#                                                             max. column:     |
#****** sge_procedures/release_job() ******
# 
#  NAME
#     release_job -- release job from hold state
#
#  SYNOPSIS
#     release_job { jobid } 
#
#  FUNCTION
#     This procedure will release the job from hold.
#
#  INPUTS
#     jobid - job identification number
#
#  RESULT
#      0   - ok
#     -1   - timeout error
#
#  SEE ALSO
#     sge_procedures/release_job()
#     sge_procedures/hold_job()
#*******************************
proc release_job { jobid } {

   global CHECK_PRODUCT_ROOT CHECK_ARCH  open_spawn_buffer CHECK_HOST CHECK_USER
 
   # spawn process
   log_user 0

   set MODIFIED_HOLD [translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_MOD_JOBS_SU] "*" "*"]
   set MODIFIED_HOLD_ARRAY [ translate $CHECK_HOST 1 0 0 [sge_macro MSG_SGETEXT_MOD_JATASK_SUU] "*" "*" "*"]

   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qrls"
   set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "$jobid" ]

   set sp_id [ lindex $id 1 ]
   set timeout 30
   set result -1	
   log_user 0

   expect {
       -i $sp_id full_buffer {
          set result -1
          add_proc_error "release_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
       }
       -i $sp_id $MODIFIED_HOLD {
          set result 0
       }
       -i $sp_id $MODIFIED_HOLD_ARRAY {
          set result 0
       }
 
       -i $sp_id "modified hold of job" {
          set result 0
       }
       -i $sp_id default {
          set result -1 
       }
   }

   # close spawned process 
   close_spawn_process $id
   log_user 1
   if { $result != 0 } {
      add_proc_error "release_job" -1 "could not release job $jobid"
   }
   return $result

}


#                                                             max. column:     |
#****** sge_procedures/wait_for_jobend() ******
# 
#  NAME
#     wait_for_jobend -- wait for end of job
#
#  SYNOPSIS
#     wait_for_jobend { jobid jobname seconds 
#                       { runcheck 1} 
#                       { wait_for_end 0 } } 
#
#  FUNCTION
#     This procedure is testing first if the given job is really running. After
#     that it waits for the job to disappear in the qstat output.
#
#  INPUTS
#     jobid   - job identification number
#     jobname - name of job
#     seconds - timeout in seconds
#
#     optional parameters:
#     { runcheck }     - if 1 (default): check if job is running
#     { wait_for_end } - if 0 (default): no for real job end waiting (job
#                                        removed from qmaster internal list)
#                        if NOT 0:       wait for qmaster to remove job from
#                                        internal list
#
#  RESULT
#      0 - job stops running
#     -1 - timeout error
#     -2 - job is not running
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
#     sge_procedures/wait_for_end_of_all_jobs()
#     sge_procedures/wait_for_load_from_all_queues()
#     file_procedures/wait_for_file()
#     sge_procedures/wait_for_jobstart()
#     sge_procedures/wait_for_end_of_transfer()
#     sge_procedures/wait_for_jobpending()
#     sge_procedures/wait_for_jobend()
#*******************************
proc wait_for_jobend { jobid jobname seconds {runcheck 1} { wait_for_end 0 } } {
  
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for end of job $jobid ($jobname)"
  
  if { $runcheck == 1 } {
     if { [is_job_running $jobid $jobname] != 1 } {
        add_proc_error "wait_for_jobend" -1 "job \"$jobid\" \"$jobname\" is not running"
        return -2
     }
  }
 
  set time [timestamp]
  while {1} {
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == -1} {
       break;
    } 
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_jobend" -1 "timeout waiting for job \"$jobid\" \"$jobname\":\nis_job_running returned $run_result"
       return -1
    }
    sleep 1
  }

  if { $wait_for_end != 0 } {
      set my_timeout [timestamp]
      incr my_timeout 90
      while { [get_qstat_j_info $jobid ] != 0 } {
          puts $CHECK_OUTPUT "waiting for jobend ..."
          sleep 2
          if { [timestamp] > $my_timeout } {
             add_proc_error "wait_for_jobend" -1 "timeout while waiting for jobend"
             break;
          }
      }
   }

  return 0
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
         if { [file isfile $CHECK_PRODUCT_ROOT/default/common/product_mode ] == 1 } {
            set product_mode_file [ open $CHECK_PRODUCT_ROOT/default/common/product_mode "r" ]
            gets $product_mode_file line
            if { $CHECK_PRODUCT_FEATURE == "csp" } {
                if { [ string first "csp" $line ] < 0 } {
                    puts $CHECK_OUTPUT "get_version_info - product feature is not csp ( secure )"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            } else {
                if { [ string first "csp" $line ] >= 0 } {
                    puts $CHECK_OUTPUT "resolve_version - product feature is csp ( secure )"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            }
            if { $CHECK_PRODUCT_TYPE == "sgeee" } {
                if { [ string first "sgeee" $line ] < 0 } {
                    puts $CHECK_OUTPUT "resolve_version - no sgeee system"
                    puts $CHECK_OUTPUT "please remove the file"
                    puts $CHECK_OUTPUT "\n$CHECK_PRODUCT_ROOT/default/common/product_mode"
                    puts $CHECK_OUTPUT "\nif you want to install a new sge system"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            } else {
                if { [ string first "sgeee" $line ] >= 0 } {
                    puts $CHECK_OUTPUT "resolve_version - this is a sgeee system"
                    puts $CHECK_OUTPUT "testsuite setup error - stop"
                    exit -1
                } 
            }
            close $product_mode_file
         }
      }  
      return $CHECK_PRODUCT_VERSION_NUMBER
   }
   set CHECK_PRODUCT_VERSION_NUMBER "system not installed - run compile option first"
   return $CHECK_PRODUCT_VERSION_NUMBER
}



#                                                             max. column:     |
#****** sge_procedures/startup_qmaster() ******
# 
#  NAME
#     startup_qmaster -- ??? 
#
#  SYNOPSIS
#     startup_qmaster { } 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc startup_qmaster {} {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_OUTPUT
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_START_SCRIPT_NAME CHECK_SCRIPT_FILE_DIR CHECK_TESTSUITE_ROOT CHECK_DEBUG_LEVEL
   global schedd_debug master_debug CHECK_DISPLAY_OUTPUT CHECK_SGE_DEBUG_LEVEL

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_qmaster" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   } 

   puts $CHECK_OUTPUT "starting up qmaster and scheduler on host \"$CHECK_CORE_MASTER\" as user \"$startup_user\""
   set arch [resolve_arch $CHECK_CORE_MASTER]

   if { $master_debug != 0 } {
      puts $CHECK_OUTPUT "using DISPLAY=${CHECK_DISPLAY_OUTPUT}"
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "/usr/openwin/bin/xterm" "-bg darkolivegreen -fg navajowhite -sl 5000 -sb -j -display $CHECK_DISPLAY_OUTPUT -e $CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/debug_starter.sh /tmp/out.$CHECK_USER.qmaster.$CHECK_CORE_MASTER \"$CHECK_SGE_DEBUG_LEVEL\" $CHECK_PRODUCT_ROOT/bin/${arch}/sge_qmaster &" prg_exit_state 60 2 ""
   } else {
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "$CHECK_PRODUCT_ROOT/bin/${arch}/sge_qmaster" ""
   }
   if { $schedd_debug != 0 } {
      puts $CHECK_OUTPUT "using DISPLAY=${CHECK_DISPLAY_OUTPUT}"
      puts $CHECK_OUTPUT "starting schedd as $startup_user" 
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "/usr/openwin/bin/xterm" "-bg darkolivegreen -fg navajowhite -sl 5000 -sb -j -display $CHECK_DISPLAY_OUTPUT -e $CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/debug_starter.sh /tmp/out.$CHECK_USER.schedd.$CHECK_CORE_MASTER \"$CHECK_SGE_DEBUG_LEVEL\" $CHECK_PRODUCT_ROOT/bin/${arch}/sge_schedd &" prg_exit_state 60 2 ""
   } else {
      puts $CHECK_OUTPUT "starting schedd as $startup_user" 
      set result [start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "$CHECK_PRODUCT_ROOT/bin/${arch}/sge_schedd" ";sleep 5"]
      puts $CHECK_OUTPUT $result
   }
   

#   set output [start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "$CHECK_PRODUCT_ROOT/default/common/$CHECK_START_SCRIPT_NAME" "-qmaster" ]

#   if { [string first "found running qmaster with pid" $output] >= 0 } {
#      add_proc_error "startup_qmaster" -1 "qmaster on host $CHECK_CORE_MASTER is allready running"
#      return -1
#   }
   return 0
}

#****** sge_procedures/startup_scheduler() *************************************
#  NAME
#     startup_scheduler() -- ??? 
#
#  SYNOPSIS
#     startup_scheduler { } 
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
#*******************************************************************************
proc startup_scheduler {} {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_OUTPUT
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_START_SCRIPT_NAME CHECK_SCRIPT_FILE_DIR CHECK_TESTSUITE_ROOT CHECK_DEBUG_LEVEL
   global schedd_debug CHECK_DISPLAY_OUTPUT CHECK_SGE_DEBUG_LEVEL

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_scheduler" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   } 

   puts $CHECK_OUTPUT "starting up scheduler on host \"$CHECK_CORE_MASTER\" as user \"$startup_user\""
   set arch [resolve_arch $CHECK_CORE_MASTER]

   if { $schedd_debug != 0 } {
      puts $CHECK_OUTPUT "using DISPLAY=${CHECK_DISPLAY_OUTPUT}"
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "/usr/openwin/bin/xterm" "-bg darkolivegreen -fg navajowhite -sl 5000 -sb -j -display $CHECK_DISPLAY_OUTPUT -e $CHECK_TESTSUITE_ROOT/$CHECK_SCRIPT_FILE_DIR/debug_starter.sh /tmp/out.$CHECK_USER.schedd.$CHECK_CORE_MASTER \"$CHECK_SGE_DEBUG_LEVEL\" $CHECK_PRODUCT_ROOT/bin/${arch}/sge_schedd &" prg_exit_state 60 2 ""
   } else {
      start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "$CHECK_PRODUCT_ROOT/bin/${arch}/sge_schedd" ""
   }
   
   return 0
}

#                                                             max. column:     |
#****** sge_procedures/startup_execd() ******
# 
#  NAME
#     startup_execd -- ??? 
#
#  SYNOPSIS
#     startup_execd { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc startup_execd { hostname } {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_OUTPUT
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_START_SCRIPT_NAME CHECK_CORE_MASTER

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_execd" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }

   puts $CHECK_OUTPUT "starting up execd on host \"$hostname\" as user \"$startup_user\""
   set output [start_remote_prog "$hostname" "$startup_user" "$CHECK_PRODUCT_ROOT/default/common/$CHECK_START_SCRIPT_NAME" "-execd"]

   set ALREADY_RUNNING [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S] "*"]

   if { [string match "*$ALREADY_RUNNING" $output ] } {
      add_proc_error "startup_execd" -1 "execd on host $hostname is allready running"
      return -1
   }

   return 0
}

#****** sge_procedures/startup_execd_raw() *************************************
#  NAME
#     startup_execd_raw() -- startup execd without using startup script
#
#  SYNOPSIS
#     startup_execd_raw { hostname } 
#
#  FUNCTION
#     Startup execd on remote host
#
#  INPUTS
#     hostname - host to start up execd
#
#  RESULT
#     0 -> ok   1 -> error
#
#  SEE ALSO
#     sge_procedures/startup_execd()
#*******************************************************************************
proc startup_execd_raw { hostname } {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_OUTPUT
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_START_SCRIPT_NAME CHECK_CORE_MASTER

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_execd" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }

   puts $CHECK_OUTPUT "starting up execd on host \"$hostname\" as user \"$startup_user\""
   set remote_arch [ resolve_arch $hostname ]
   set my_environment(COMMD_HOST) $CHECK_CORE_MASTER
   set output [start_remote_prog "$hostname" "$startup_user" "$CHECK_PRODUCT_ROOT/bin/$remote_arch/sge_execd" "-nostart-commd" prg_exit_state 60 0 my_environment 1 1 1]


   set ALREADY_RUNNING [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S] "*"]

   if { [string match "*$ALREADY_RUNNING" $output ] } {
      add_proc_error "startup_execd" -1 "execd on host $hostname is allready running"
      return -1
   }
   return 0
}

#                                                             max. column:     |
#****** sge_procedures/startup_shadowd() ******
# 
#  NAME
#     startup_shadowd -- ??? 
#
#  SYNOPSIS
#     startup_shadowd { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc startup_shadowd { hostname } {
   global CHECK_PRODUCT_TYPE CHECK_PRODUCT_ROOT CHECK_OUTPUT
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_START_SCRIPT_NAME


   if { $CHECK_ADMIN_USER_SYSTEM == 0 } {  
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_shadowd" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }
 

   puts $CHECK_OUTPUT "starting up shadowd on host \"$hostname\" as user \"$startup_user\""

   set output [start_remote_prog "$hostname" "$startup_user" "$CHECK_PRODUCT_ROOT/default/common/$CHECK_START_SCRIPT_NAME" "-shadowd"]
   puts $CHECK_OUTPUT $output
   if { [string first "starting sge_shadowd" $output] >= 0 } {
       return 0
   }
   add_proc_error "startup_shadowd" -1 "could not start shadowd on host $hostname:\noutput:\"$output\""
   return -1
}

# return values: 
# 3 - master and scheduler are running
# 2 - master is running, scheduler is not running
# 1 - master is not running, scheduler is running
# 0 - master and scheduler are not running

#                                                             max. column:     |
#****** sge_procedures/are_master_and_scheduler_running() ******
# 
#  NAME
#     are_master_and_scheduler_running -- ??? 
#
#  SYNOPSIS
#     are_master_and_scheduler_running { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
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
proc are_master_and_scheduler_running { hostname qmaster_spool_dir } {
 
   global CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_ROOT

   set qmaster_pid -1
   set scheduler_pid -1

   set running 0


   set qmaster_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/qmaster.pid" ]
   set qmaster_pid [ string trim $qmaster_pid ]
   if { $prg_exit_state != 0 } {
      set qmaster_pid -1
   }

   set scheduler_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/schedd/schedd.pid" ]
   set scheduler_pid [ string trim $scheduler_pid ]
   if { $prg_exit_state != 0 } {
      set scheduler_pid -1
   }


   get_ps_info $qmaster_pid $hostname

   if { ($ps_info($qmaster_pid,error) == 0) && ( [ string first "qmaster" $ps_info($qmaster_pid,string)] >= 0 )  } {
      incr running 2
   }

   get_ps_info $scheduler_pid $hostname

   if { ($ps_info($scheduler_pid,error) == 0) && ( [ string first "schedd" $ps_info($scheduler_pid,string)] >= 0  ) } {
      incr running 1
   }

   return $running
}


# kills master and scheduler on the given hostname
#                                                             max. column:     |
#****** sge_procedures/shutdown_master_and_scheduler() ******
# 
#  NAME
#     shutdown_master_and_scheduler -- ??? 
#
#  SYNOPSIS
#     shutdown_master_and_scheduler { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_master_and_scheduler {hostname qmaster_spool_dir} {
   shutdown_scheduler $hostname $qmaster_spool_dir
   shutdown_qmaster $hostname $qmaster_spool_dir
}

#****** sge_procedures/shutdown_scheduler() ************************************
#  NAME
#     shutdown_scheduler() -- ??? 
#
#  SYNOPSIS
#     shutdown_scheduler { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
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
#*******************************************************************************
proc shutdown_scheduler {hostname qmaster_spool_dir} {
   global CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_ROOT CHECK_ADMIN_USER_SYSTEM
   global CHECK_PRODUCT_TYPE CHECK_ARCH

   puts $CHECK_OUTPUT "shutdown_scheduler ..."

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "killing scheduler on host $hostname ..."
   puts $CHECK_OUTPUT "retrieving data from spool dir $qmaster_spool_dir"



   set scheduler_pid [ get_scheduler_pid $hostname $qmaster_spool_dir ]

   get_ps_info $scheduler_pid $hostname
   if { ($ps_info($scheduler_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $scheduler_pid "sge_schedd" ] == 0 } { 
         puts $CHECK_OUTPUT "killing schedd with pid $scheduler_pid on host $hostname"

         catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ks" } result

         shutdown_system_daemon $hostname sched

      } else {
         add_proc_error "shutdown_scheduler" "-1" "scheduler pid $scheduler_pid not found"
         set scheduler_pid -1
      }
   } else {
      add_proc_error "shutdown_scheduler" "-1" "ps_info failed (1), pid=$scheduler_pid"
      set scheduler_pid -1
   }

   puts $CHECK_OUTPUT "done."
}  
#****** sge_procedures/is_scheduler_alive() ************************************
#  NAME
#     is_scheduler_alive() -- ??? 
#
#  SYNOPSIS
#     is_scheduler_alive { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
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
#*******************************************************************************
proc is_scheduler_alive { hostname qmaster_spool_dir } {

   set scheduler_pid [get_scheduler_pid $hostname $qmaster_spool_dir]
   get_ps_info $scheduler_pid $hostname
   
   set alive 0
   if { ($ps_info($scheduler_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $scheduler_pid "sge_schedd" ] == 0 } { 
         set alive 1
      }
   }

   return $alive
}

#****** sge_procedures/get_scheduler_pid() *************************************
#  NAME
#     get_scheduler_pid() -- ??? 
#
#  SYNOPSIS
#     get_scheduler_pid { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
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
#*******************************************************************************
proc get_scheduler_pid { hostname qmaster_spool_dir } {
   global CHECK_USER 

   set scheduler_pid -1
   set scheduler_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/schedd/schedd.pid" ]
   set scheduler_pid [ string trim $scheduler_pid ]
   if { $prg_exit_state != 0 } {
      set scheduler_pid -1
   }
   return $scheduler_pid
}

#****** sge_procedures/shutdown_qmaster() **************************************
#  NAME
#     shutdown_qmaster() -- ??? 
#
#  SYNOPSIS
#     shutdown_qmaster { hostname qmaster_spool_dir } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname          - ??? 
#     qmaster_spool_dir - ??? 
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
#*******************************************************************************
proc shutdown_qmaster {hostname qmaster_spool_dir} {
   global CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_ROOT CHECK_ADMIN_USER_SYSTEM
   global CHECK_PRODUCT_TYPE CHECK_ARCH

   puts $CHECK_OUTPUT "shutdown_qmaster ..."

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "killing qmaster on host $hostname ..."
   puts $CHECK_OUTPUT "retrieving data from spool dir $qmaster_spool_dir"



   set qmaster_pid -1

   set qmaster_pid [ start_remote_prog "$hostname" "$CHECK_USER" "cat" "$qmaster_spool_dir/qmaster.pid" ]
   set qmaster_pid [ string trim $qmaster_pid ]
   if { $prg_exit_state != 0 } {
      set qmaster_pid -1
   }

   get_ps_info $qmaster_pid $hostname
   if { ($ps_info($qmaster_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $qmaster_pid "sge_qmaster" ] == 0 } { 

         puts $CHECK_OUTPUT "killing qmaster with pid $qmaster_pid on host $hostname"

         catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-km" } result

         shutdown_system_daemon $hostname qmaster

      } else {
         add_proc_error "shutdown_qmaster" "-1" "qmaster pid $qmaster_pid not found"
         set qmaster_pid -1
      }
   } else {
      add_proc_error "shutdown_qmaster" "-1" "ps_info failed (2), pid=$qmaster_pid"
      set qmaster_pid -1
   }
   puts $CHECK_OUTPUT "done."
}  

#                                                             max. column:     |
#****** sge_procedures/shutdown_all_shadowd() ******
# 
#  NAME
#     shutdown_all_shadowd -- ??? 
#
#  SYNOPSIS
#     shutdown_all_shadowd { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_all_shadowd { hostname } {
   global CHECK_PRODUCT_ROOT CHECK_OUTPUT CHECK_ADMIN_USER_SYSTEM
   global CHECK_USER CHECK_PRODUCT_TYPE

   set num_proc 0

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "shutdown all shadowd daemon for system installed at $CHECK_PRODUCT_ROOT ..."

   set index_list [ ps_grep "$CHECK_PRODUCT_ROOT" "$hostname" ]
   set new_index ""
   foreach elem $index_list {
      if { [ string first "shadowd" $ps_info(string,$elem) ] >= 0 } {
         lappend new_index $elem
      }
   } 
   set num_proc [llength $new_index]
   puts $CHECK_OUTPUT "Number of matching processes: $num_proc"
   foreach elem $new_index {
      puts $CHECK_OUTPUT $ps_info(string,$elem)
      if { [ is_pid_with_name_existing $hostname $ps_info(pid,$elem) "sge_shadowd" ] == 0 } {
         puts $CHECK_OUTPUT "Killing process [ set ps_info(pid,$elem) ] ..."
         if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
             start_remote_prog "$hostname" "root" "kill" "$ps_info(pid,$elem)"
         } else {
             start_remote_prog "$hostname" "$CHECK_USER" "kill" "$ps_info(pid,$elem)"
         }
      } else {
         add_proc_error "shutdown_all_shadowd" "-2" "could not shutdown all shadowd daemons"
      }
   }

   return $num_proc
}

#
#                                                             max. column:     |
#
#****** sge_procedures/is_pid_with_name_existing() ******
#  NAME
#     is_pid_with_name_existing -- search for process on remote host 
#
#  SYNOPSIS
#     is_pid_with_name_existing { host pid proc_name } 
#
#  FUNCTION
#     This procedure will start the checkprog binary with the given parameters. 
#     
#
#  INPUTS
#     host      - remote host 
#     pid       - pid of process 
#     proc_name - process program name 
#
#  RESULT
#     0 - ok; != 0 on error 
#
#  SEE ALSO
#     sge_procedures/is_job_running()
#     sge_procedures/is_pid_with_name_existing()
#*******************************
#
proc is_pid_with_name_existing { host pid proc_name } {
  global CHECK_OUTPUT CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_USER

  puts $CHECK_OUTPUT "$host: checkprog $pid $proc_name ..."
  set my_arch [ resolve_arch $host ]
  puts $CHECK_OUTPUT [start_remote_prog $host $CHECK_USER $CHECK_PRODUCT_ROOT/utilbin/$my_arch/checkprog "$pid $proc_name"]
  puts $CHECK_OUTPUT "return: $prg_exit_state"
  return $prg_exit_state
}


#
#                                                             max. column:     |
#
#****** sge_procedures/shutdown_system_daemon() ******
#  NAME
#     shutdown_system_daemon -- kill running sge daemon 
#
#  SYNOPSIS
#     shutdown_system_daemon { host type } 
#
#  FUNCTION
#     This procedure will kill all commd, execd, qmaster or sched processes on 
#     the given host. It does not matter weather the system is sgeee or sge
#     (sge or sgeee). 
#
#  INPUTS
#     host     - remote host 
#     typelist - list of processes to kill (commd, execd, qmaster or sched)
#
#  RESULT
#     none 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
#
proc shutdown_system_daemon { host typelist } {
global CHECK_HOST CHECK_ARCH CHECK_PRODUCT_ROOT CHECK_PRODUCT_TYPE CHECK_CORE_EXECD 
global CHECK_CORE_MASTER CHECK_CORE_INSTALLED CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_TYPE
global CHECK_ADMIN_USER_SYSTEM


   puts $CHECK_OUTPUT "shutdown_system_daemon ... ($host/$typelist)" 
   set process_names ""
   foreach type $typelist {
      if { [ string compare $type "execd" ] == 0 } {
         lappend process_names "sge_execd" 
      }
      if { [ string compare $type "sched" ] == 0 } {
         lappend process_names "sge_schedd" 
      }
      if { [ string compare $type "qmaster" ] == 0 } {
         lappend process_names "sge_qmaster" 
      }
      if { [ string compare $type "commd" ] == 0 } {
         lappend process_names "sge_commd" 
      }
   }

   if { [llength $process_names] != [llength $typelist] } {
      add_proc_error "shutdown_system_daemon" -1 "type should be commd, execd, qmaster or sched"
      puts $CHECK_OUTPUT "shutdown_system_daemon ... done"
      return -1
   }

   set found_p [ ps_grep "$CHECK_PRODUCT_ROOT/" $host ]

   foreach process_name $process_names {

      puts $CHECK_OUTPUT "looking for \"$process_name\" processes on host $host ..."
      foreach elem $found_p {
         if { [ string first $process_name $ps_info(string,$elem) ] >= 0 } {
            if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
               puts $CHECK_OUTPUT "found running $process_name with pid $ps_info(pid,$elem) on host $host"
               puts $CHECK_OUTPUT $ps_info(string,$elem)
               if { [ have_root_passwd ] == -1 } {
                   set_root_passwd 
               }
               if { $CHECK_ADMIN_USER_SYSTEM == 0 } {
                   set kill_user "root"  
               } else {
                   set kill_user $CHECK_USER
               }
               puts $CHECK_OUTPUT "killing process $ps_info(pid,$elem) on host $host, kill user is $kill_user"
               puts $CHECK_OUTPUT [ start_remote_prog $host $kill_user kill $ps_info(pid,$elem) ]
               sleep 5
               if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
                   puts $CHECK_OUTPUT "killing (SIG_KILL) process $ps_info(pid,$elem) on host $host, kill user is $kill_user"
                   puts $CHECK_OUTPUT [ start_remote_prog $host $kill_user kill "-9 $ps_info(pid,$elem)" ]
                   sleep 5
                   if { [ is_pid_with_name_existing $host $ps_info(pid,$elem) $process_name ] == 0 } {
                       puts $CHECK_OUTPUT "pid:$ps_info(pid,$elem) kill failed (host: $host)"
                       add_proc_error "" -1 "could not shutdown \"$process_name\" on host $host"
                   } else {
                       puts $CHECK_OUTPUT "pid:$ps_info(pid,$elem) process killed (host: $host)"
                   }
               } else {
                   puts $CHECK_OUTPUT "pid:$ps_info(pid,$elem) process killed (host: $host)"
               }
            } else {
               puts $CHECK_OUTPUT "checkprog error"
               add_proc_error "" -1 "could not shutdown \"$process_name\" on host $host"
            }
         }
      }
   }
   puts $CHECK_OUTPUT "shutdown_system_daemon ... done"
   return 0
}

#                                                             max. column:     |
#****** sge_procedures/shutdown_core_system() ******
# 
#  NAME
#     shutdown_core_system -- ??? 
#
#  SYNOPSIS
#     shutdown_core_system { } 
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
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc shutdown_core_system {} {
global CHECK_ARCH 
global CHECK_PRODUCT_ROOT 
global CHECK_CORE_EXECD 
global CHECK_CORE_MASTER 
global CHECK_OUTPUT
global CHECK_USER
global CHECK_ADMIN_USER_SYSTEM do_compile

   puts $CHECK_OUTPUT "killing qmaster, scheduler and all execds in the cluster ..."

   set result ""
   set do_ps_kill 0
   set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ke all -ks -km" ]

   puts $CHECK_OUTPUT "qconf -ke all -ks -km returned $prg_exit_state"
   if { $prg_exit_state == 0 } {
      puts $CHECK_OUTPUT $result
   } else {
      set do_ps_kill 1
      puts $CHECK_OUTPUT "shutdown_core_system - qconf error or binary not found\n$result"
   }

   sleep 5  ;# give the qmaster time
   puts $CHECK_OUTPUT "killing all commds in the cluster ..." 
  
   set do_it_as_root 0
   foreach elem $CHECK_CORE_EXECD { 
       puts $CHECK_OUTPUT "killing commd on host $elem"
       if { $do_it_as_root == 0 } { 
          set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/sgecommdcntl" "-U -k -host $elem"  ]
       } else {
          set result [ start_remote_prog "$CHECK_CORE_MASTER" "root" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/sgecommdcntl" "-k -host $elem"  ]
       } 
       if { $prg_exit_state == 0 } {
          puts $CHECK_OUTPUT $result
       } else {
          puts $CHECK_OUTPUT $result
          if { $prg_exit_state == 255 } {
             puts $CHECK_OUTPUT "\"sgecommdcntl -k\" must be started by root user (to get reserved port)!"
             puts $CHECK_OUTPUT "try again as root user ..." 
             if { [ have_root_passwd ] == -1 } {
                set_root_passwd 
             }
             if { $CHECK_ADMIN_USER_SYSTEM != 1 } {
                set result [ start_remote_prog "$CHECK_CORE_MASTER" "root" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/sgecommdcntl" "-k -host $elem"  ]
             }
          }
          if { $prg_exit_state == 0 } {
             set do_it_as_root 1 
             puts $CHECK_OUTPUT $result
             puts $CHECK_OUTPUT "sgecommdcntl -k -host $elem - success"
          } else {
             set do_ps_kill 1
             puts $CHECK_OUTPUT "shutdown_core_system - commdcntl error or binary not found"
          }
       }
   }
   
   if { $do_ps_kill == 1 && $do_compile == 0} {
      puts $CHECK_OUTPUT "perhaps master is not running, trying to shutdown cluster with ps information"

      set hosts_to_check $CHECK_CORE_MASTER
      foreach elem $CHECK_CORE_EXECD {
         if { [ string first $elem $hosts_to_check ] < 0 } {
            lappend hosts_to_check $elem
         }
      }
      set proccess_names "sched"
      lappend proccess_names "execd"
      lappend proccess_names "qmaster"
      lappend proccess_names "commd"

      foreach host $hosts_to_check { 
            puts $CHECK_OUTPUT ""
            shutdown_system_daemon $host $proccess_names
      }
   }
}

#                                                             max. column:     |
#****** sge_procedures/gethostname() ******
# 
#  NAME
#     gethostname -- ??? 
#
#  SYNOPSIS
#     gethostname { } 
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
proc gethostname {} {
  global CHECK_PRODUCT_ROOT CHECK_ARCH  CHECK_OUTPUT env

  set catch_return [ catch { exec "$CHECK_PRODUCT_ROOT/utilbin/$CHECK_ARCH/gethostname" "-name"} result ]
  if { $catch_return == 0 } {
     set result [split $result "."]
     set newname [lindex $result 0]
     return $newname
  } else {
     debug_puts "proc gethostname - gethostname error or binary not found"
     debug_puts "error: $result"
     debug_puts "error: $catch_return"
     debug_puts "trying local hostname call ..."
     set catch_return [ catch { exec "hostname" } result ]
     if { $catch_return == 0 } {
        set result [split $result "."]
        set newname [lindex $result 0]
        debug_puts "got hostname: \"$newname\""
        return $newname
     } else {
        debug_puts "local hostname error or binary not found"
        debug_puts "error: $result"
        debug_puts "error: $catch_return"
        debug_puts "trying local HOST environment variable ..."
        if { [ info exists env(HOST) ] } {
           set result [split $env(HOST) "."]
           set newname [lindex $result 0]
           if { [ string length $newname ] > 0 } {
               debug_puts "got hostname_ \"$newname\""
               return $newname
           } 
        }
     }
     return "unknown"
  }
} 

#                                                             max. column:     |
#****** sge_procedures/resolve_arch() ******
# 
#  NAME
#     resolve_arch -- ??? 
#
#  SYNOPSIS
#     resolve_arch { { host "none" } } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     { host "none" } - ??? 
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
proc resolve_arch { { host "none" } } {
  global CHECK_PRODUCT_ROOT CHECK_OUTPUT CHECK_TESTSUITE_ROOT arch_cache
  global CHECK_SCRIPT_FILE_DIR CHECK_USER CHECK_SOURCE_DIR CHECK_HOST

  if { [ info exists arch_cache($host) ] } {
     return $arch_cache($host)
  }

  if { [ info exists CHECK_USER ] == 0 } {
     puts $CHECK_OUTPUT "user not set, aborting"
     return "unknown"
  }
  
  if { [ info exists CHECK_SOURCE_DIR ] == 0 } {
     debug_puts "source directory not set, aborting"
     return "unknown"
  }

 

  if { [ string compare $host "none" ] == 0 || 
       [ string compare $host $CHECK_HOST ] == 0 } {
      set prg_exit_state [ catch { eval exec "$CHECK_SOURCE_DIR/dist/util/arch" } result ]
  } else {
      debug_puts "resolve_arch: resolving architecture for host $host"
      set result [ start_remote_prog $host $CHECK_USER "$CHECK_SOURCE_DIR/dist/util/arch" "" prg_exit_state 60 0 "" 1 0 0]
  }
  set result [string trim $result]
  set result2 [split $result "\n"]
  if { [ llength $result2 ] > 1 } {
     puts $CHECK_OUTPUT "util/arch script returns more than 1 line output ..."
     foreach elem $result2  {
        puts $CHECK_OUTPUT "\"$elem\""
        if { [string first " " $elem ] < 0  } {
           set result $elem
           puts $CHECK_OUTPUT "using \"$result\" as architecture"
           break
        }
     }
  }
  if { [ llength $result2 ] < 1 } {
      puts $CHECK_OUTPUT "util/arch script returns no value ..."
      return "unknown"
  }
  if { [string first ":" $result] >= 0 } {
     puts $CHECK_OUTPUT "architecture or file \"$CHECK_SOURCE_DIR/dist/util/arch\" not found"
     return "unknown"
  }
  set result [lindex $result 0]  ;# remove CR

  if { [ string compare $result "" ] == 0 } {
     puts $CHECK_OUTPUT "architecture or file \"$CHECK_SOURCE_DIR/dist/util/arch\" not found"
     return "unknown"
  } 

  set arch_cache($host) [lindex $result 0]
  
  if { [info exists arch_cache($host) ] != 1 } {
     return "unknown"
  }

  return $arch_cache($host)
}

#                                                             max. column:     |
#****** sge_procedures/resolve_upper_arch() ******
# 
#  NAME
#     resolve_upper_arch -- ??? 
#
#  SYNOPSIS
#     resolve_upper_arch { host } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     host - ??? 
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
proc resolve_upper_arch { host } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT CHECK_TESTSUITE_ROOT upper_arch_cache CHECK_SOURCE_DIR
  global CHECK_USER
  if { [info exists upper_arch_cache($host) ] } {
     return $upper_arch_cache($host)
  }

  set result [ start_remote_prog $host $CHECK_USER "cd" "$CHECK_SOURCE_DIR ; ./aimk -no-mk" prg_exit_state 60 0 "" 1 0]
 
  set result [split $result "\n"]
  set result [join $result ""]
  set result [split $result "\r"]
  set result [join $result ""]

  if { $prg_exit_state != 0 } {
     add_proc_error "resolve_upper_arch" "-1" "architecture not found or aimk not found in $CHECK_SOURCE_DIR"
     return ""
  }
  set upper_arch_cache($host) $result
  puts $CHECK_OUTPUT "upper arch is \"$result\""

  return $upper_arch_cache($host)
}


#                                                             max. column:     |
#****** sge_procedures/resolve_host() ******
# 
#  NAME
#     resolve_host -- ??? 
#
#  SYNOPSIS
#     resolve_host { name { long 0 } } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     name       - ??? 
#     { long 0 } - ??? 
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
proc resolve_host { name { long 0 } } {

  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT CHECK_TESTSUITE_ROOT 
  global CHECK_SCRIPT_FILE_DIR CHECK_USER

  set remote_arch [ resolve_arch $name ]

  set result [ start_remote_prog $name $CHECK_USER "$CHECK_PRODUCT_ROOT/utilbin/$remote_arch/gethostname" "-name" prg_exit_state 60 0 "" 0 ]
  set result [ lindex $result 0 ]  ;# removing /r /n

  if { $prg_exit_state != 0 } {
     puts $CHECK_OUTPUT "proc resolve_host - gethostname error or file \"$CHECK_PRODUCT_ROOT/utilbin/$remote_arch/gethostname\" not found"
     return "unknown"
  }

  set newname $result
  if { $long == 0 } {
     set result [split $result "."]
     set newname [lindex $result 0]
  }
  puts $CHECK_OUTPUT "\"$name\" resolved to \"$newname\""
  return $newname
}





# main
if { [info exists argc ] != 0 } {
   set TS_ROOT ""
   set procedure ""
   for { set i 0 } { $i < $argc } { incr i } {
      if {$i == 0} { set TS_ROOT [lindex $argv $i] }
      if {$i == 1} { set procedure [lindex $argv $i] }
   }
   if { $argc == 0 } {
      puts "usage:\n$module_name <CHECK_TESTSUITE_ROOT> <proc> no_main <testsuite params>"
      puts "options:"
      puts "CHECK_TESTSUITE_ROOT -  path to TESTSUITE directory"
      puts "proc                 -  procedure from this file with parameters"
      puts "no_main              -  used to source testsuite file (check.exp)"
      puts "testsuite params     -  any testsuite command option (from file check.exp)"
      puts "                        testsuite params: file <path>/defaults.sav is needed"
   } else {
      source "$TS_ROOT/check.exp"
      puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
      puts $CHECK_OUTPUT "calling \"$procedure\" ..."
      set result [ eval $procedure ]
      puts $result 
      flush $CHECK_OUTPUT
   }
}

