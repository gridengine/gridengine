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

   global CHECK_PRODUCT_VERSION_NUMBER

   
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

  upvar $change_array chgar

  set values [array names chgar]

  get_exechost old_values $host

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal $chgar($elem)
   
     if {[info exists old_values($elem)]} {
        set newVal1 [split $newVal {/}]
        set newVal [join $newVal1 {\/}]
        lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
     } else {
        lappend vi_commands "A\n$elem  $newVal"
        lappend vi_commands [format "%c" 27]
     }
  } 
  set result [handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-me $host" $vi_commands "modified" "changed"]
  if { $result == -2 } {
     set result 0
  }
  if { $result != 0 } {
     add_proc_error "set_exechost" -1 "could not modifiy exechost $host"
     set result -1
  }
  return $result
}




#                                                             max. column:     |
#****** sge_procedures/get_loadsensor_path() ******
# 
#  NAME
#     get_loadsensor_path -- ??? 
#
#  SYNOPSIS
#     get_loadsensor_path { arch } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     arch - ??? 
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
proc get_loadsensor_path { arch } {
   global CHECK_LOADSENSOR_DIR_CONFIG_FILE CHECK_CONFIG_DIR CHECK_OUTPUT

   set config "$CHECK_CONFIG_DIR/$CHECK_LOADSENSOR_DIR_CONFIG_FILE"  
   set sensor_path ""

   if { [file exists $config] } {
      set file_p [ open $config r ]
      set line_no 0
      while { [gets $file_p line] >= 0 } {
         if { [string first "#" $line ] == 0 } {
#            puts $CHECK_OUTPUT "found comment in line $line_no"
            continue
         }
         set tmp_arch [ lindex $line 0 ]
         set tmp_path [ lindex $line 1 ]

#         puts $CHECK_OUTPUT "\"$tmp_arch\" \"$tmp_path\""
         if { [ string compare $tmp_arch $arch] == 0 } {
#            puts $CHECK_OUTPUT "found matching architecture entry $tmp_arch"
            set sensor_path $tmp_path
#            puts $CHECK_OUTPUT "path is \"$tmp_path\""
         }
         incr line_no 1
      }
      close $file_p 
   } else {
     add_proc_error "get_loadsensor_path" -1 "config file \"$config\" not found"
   }
   return $sensor_path
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

  global CHECK_CONFIG_DIR CHECK_OUTPUT

  set config "$CHECK_CONFIG_DIR/gid-range.conf"
  set range "13001-13500"
  set range_step 200
  set used_ranges ""
  
  if { [file exists $config] } {
    set file_p [ open $config r ]
    set line_no 0
    while { [gets $file_p line ] >= 0 } {
       if { [string first "#" $line ] == 0 } {
          debug_puts "found comment in line $line_no"
          incr line_no 1
          continue
       }
       if { [string first "|" $line ] > 0 } {
          set help [split $line "|"]
         
          set gidlist([lindex $help 0],[lindex $help 1]) [lindex $help 2] 
          lappend used_ranges [lindex $help 2] 
          debug_puts "found entry in line $line_no"
       }
       incr line_no 1 
    }
    close $file_p
    set names [array names gidlist]
    foreach elem $names {
       debug_puts "\"$elem\" has gid: $gidlist($elem)"
    }
    if { [ info exists gidlist($user,$port)] } {
       puts $CHECK_OUTPUT "found entry for $user on port $port: gid-gange is $gidlist($user,$port)"
       return $gidlist($user,$port)
    } else {
       set highest_val 20000
       foreach elem $used_ranges {
          set help [split $elem "-"]
          set low_val  [lindex $help 0]
          set high_val [lindex $help 1]
          if { $high_val > $highest_val } {
             set highest_val $high_val
          }
       }
       set new_range [expr ( $highest_val + 1 ) ]
       set new_end [expr ( $new_range + [ expr ( $range_step - 1 ) ] ) ]
       append new_range "-"
       append new_range $new_end
       puts $CHECK_OUTPUT "creating new range for $user on port $port: $new_range"
       set file_p [open $config "a"]
       puts $file_p "$user|$port|$new_range"
       close $file_p
       return $new_range
    }
  } else {
    puts "file does not exist, createing new one"
    # file does not exist, create new file with new entry
    set file_p [open $config "w"]
    puts $file_p "# gid range configuration file"
    puts $file_p "# each line is one entry:"
    puts $file_p "# user      port        range"
    puts $file_p "$user|$port|$range"
    close $file_p
    return $range
  }
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
  global CHECK_CORE_MASTER CHECK_OUTPUT CHECK_USER

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
  set change_array(qmaster_spool_dir) $new_spool_dir
  set_config change_array "global"
  shutdown_master_and_scheduler $CHECK_CORE_MASTER $old_spool_dir
 
  # now copy the entries  
  set result [ start_remote_tcl_prog $CHECK_CORE_MASTER $CHECK_USER "file_procedures.tcl" "copy_directory" "$old_spool_dir $new_spool_dir" ]
  if { [ string first "no errors" $result ] < 0 } {
      add_proc_error "shadowd_kill_master_and_scheduler" -1 "error moving qmaster spool dir"
  }

  startup_qmaster
  wait_for_load_from_all_queues 300
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
#     maxgjobs                    "0"
#     queue_sort_method           "load"
#     user_sort                   "true"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     SGEEE differences:
#     queue_sort_method           "share"
#     user_sort                   "false"
#     sgeee_schedule_interval       "00:01:00"
#     halftime                    "0"
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
proc reset_schedd_config {} {
  global CHECK_PRODUCT_TYPE
 
  set default_array(algorithm)                  "default"
  set default_array(schedule_interval)          "0:0:15"
  set default_array(maxujobs)                   "0"
  set default_array(maxgjobs)                   "0"
  set default_array(queue_sort_method)          "share"
  set default_array(user_sort)                  "false"
  set default_array(job_load_adjustments)       "np_load_avg=0.50"
  set default_array(load_adjustment_decay_time) "0:7:30"
  set default_array(load_formula)               "np_load_avg"
  set default_array(schedd_job_info)            "true"

# this is sgeee
  if { [string compare $CHECK_PRODUCT_TYPE "sgeee"] == 0 } {
     set default_array(sgeee_schedule_interval)      "00:01:00"
     set default_array(halftime)                   "0"
     set default_array(usage_weight_list)          "cpu=0.34,mem=0.33,io=0.33"
     set default_array(compensation_factor)        "5"
     set default_array(weight_user)                "0"
     set default_array(weight_project)             "0"
     set default_array(weight_jobclass)            "0"
     set default_array(weight_department)          "0"
     set default_array(weight_job)                 "0"
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
proc set_config { change_array {host global} {do_add 0}} {
  global env CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer
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
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-mconf $host" $vi_commands "modified" "edit failed" "added" ]
  if { ($result != 0) &&  ($result != -3) } {
     add_proc_error "set_config" -1 "could not add or modify configruation for host $host ($result)"
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
#     maxgjobs                    "0"
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
#     halftime                    "0"
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
  global CHECK_OUTPUT
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

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-msconf" $vi_commands "changed scheduler configuration" ]  

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
#     maxgjobs                    "0"
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
#     halftime                    "0"
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
#     change_array(max_migr_time)        "0"
#     change_array(migr_load_thresholds) "np_load_avg=5.00"
#     change_array(max_no_migr)          "00:02:00"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(klog)                 "/usr/local/bin/klog"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(reauth_time)          "01:40:00"
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
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-mq ${q_name}" $vi_commands "modified" "not a queuename"]
  if { $result == -2 } {
    add_proc_error "set_queue" -1 "$q_name is not a queue"
  }
  if { $result != 0  } {
    add_proc_error "set_queue" -1 "error modify queue $q_name"
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
#     change_array(max_migr_time)        "0"
#     change_array(migr_load_thresholds) "np_load_avg=5.00"
#     change_array(max_no_migr)          "00:02:00"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(klog)                 "/usr/local/bin/klog"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(reauth_time)          "01:40:00"
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

  upvar $change_array chgar
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
     set default_array(max_migr_time)        "0"
     set default_array(migr_load_thresholds) "np_load_avg=10.00"
     set default_array(max_no_migr)          "00:02:00"
     set default_array(min_cpu_interval)     "00:05:00"
     set default_array(processors)           "UNDEFINED"
     set default_array(qtype)                "BATCH INTERACTIVE CHECKPOINTING PARALLEL"
     set default_array(rerun)                "FALSE"
     set default_array(slots)                "10"
     set default_array(tmpdir)               "/tmp"
     set default_array(shell)                "/bin/csh"
     set default_array(shell_start_mode)     "NONE"
     set default_array(klog)                 "/usr/local/bin/klog"
     set default_array(prolog)               "NONE"
     set default_array(epilog)               "NONE"
     set default_array(starter_method)       "NONE"
     set default_array(suspend_method)       "NONE"
     set default_array(resume_method)        "NONE"
     set default_array(terminate_method)     "NONE"
     set default_array(reauth_time)          "01:40:00"
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
     if { [string first "added" $result ] < 0 } {
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

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-aq" $vi_commands "added" "already exists" ]  
  if { $result != 0 } {
     add_proc_error "add_queue" -1 "could not add queue [set chgar(qname)] (error: $result)"
  }
  return $result
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
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

  set result ""
  set catch_return [ catch {  
      eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf -dq ${q_name}" 
  } result ]

  if { [string first "removed" $result ] < 0 } {
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
#     change_array(max_migr_time)        "0"
#     change_array(migr_load_thresholds) "np_load_avg=5.00"
#     change_array(max_no_migr)          "00:02:00"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(klog)                 "/usr/local/bin/klog"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(reauth_time)          "01:40:00"
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
     set chgar($id) $value
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
 global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

  log_user 0 
  set timeout 30

  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod -s $qname"
  set sid [ open_spawn_process $program  ]     
  set sp_id [ lindex $sid 1 ]
  set result -1	
  log_user 0 
  expect {
     -i $sp_id full_buffer {
         set result -1
         add_proc_error "suspend_queue" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
     }
     -i $sp_id "was suspended" {
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
   global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

  set timeout 30
  log_user 0 
   
  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod -us $queue"
  set sid [ open_spawn_process $program  ]     
  set sp_id [ lindex $sid 1 ]
  set result -1	
  log_user 0 

  expect {
      -i $sp_id full_buffer {
         set result -1
         add_proc_error "unsuspend_queue" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
      }
      -i $sp_id "unsuspended queue" {
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
  global CHECK_OUTPUT CHECK_HOST CHECK_USER
  
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

  set my_ckpt_name [set chgar(ckpt_name)]
  set my_args "-ackpt $my_ckpt_name"
 
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" $my_args $vi_commands "added" "already exists" "referenced in queue list of checkpoint*does not exist"] 
  
  if { $result == -1 } { add_proc_error "add_checkpointobj" -1 "timeout error" }
  if { $result == -2 } { add_proc_error "add_checkpointobj" -1 "already exists" }
  if { $result == -3 } { add_proc_error "add_checkpointobj" -1 "queue reference does not exist" }
  if { $result != 0  } { add_proc_error "add_checkpointobj" -1 "could nod add checkpoint object" }

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
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer
  
  log_user 0
  set id [ open_spawn_process "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dckpt" "$checkpoint_name"]
  set sp_id [ lindex $id 1 ]
  set timeout 30
  set result -1 
  	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_checkpointobj" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
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
#     add_pe { change_array } 
#
#  FUNCTION
#     This procedure will create a new pe (parallel environemnt) definition 
#     object.
#
#  INPUTS
#     change_array - name of an array variable that will be set by add_pe
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
proc add_pe { change_array } {
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

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ap [set chgar(pe_name)]" $vi_commands "added" "already exists" "does not exist" ]
  
  if {$result == -1 } { add_proc_error "add_pe" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_pe" -1 "parallel environment \"[set chgar(pe_name)]\" already exists" }
  if {$result == -3 } { add_proc_error "add_pe" -1 "something (perhaps a queue) does not exist" }
  if {$result != 0  } { add_proc_error "add_pe" -1 "could not add parallel environment \"[set chgar(pe_name)]\"" }

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

  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-aprj" $vi_commands "added" "already exists" ]
  
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
   
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

  log_user 0
  set id [ open_spawn_process "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dp" "$mype_name"]
  set sp_id [ lindex $id 1 ]

  set result -1
  set timeout 30 	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_pe" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
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
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer CHECK_PRODUCT_TYPE

  if { [ string compare $CHECK_PRODUCT_TYPE "cod" ] == 0 } {
     set_error -1 "del_prj - not possible for codine systems"
     return
  }

  log_user 0
  set id [ open_spawn_process "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dprj" "$myprj_name"]
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
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

  log_user 0
  set id [ open_spawn_process "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-dcal" "$mycal_name"]
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
  # xyz is neccessary, as there is no template for calendards existing!!!
  set result [ handle_vi_edit "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-acal xyz" $vi_commands "added" "already exists"]
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
     sleep 2
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
      sleep 5
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
      sleep 5
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
             add_proc_error "wait_for_end_of_all_jobs" -1 "timeout waiting for load values < 99"
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
  
  global CHECK_PRODUCT_ROOT CHECK_ARCH

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
     set tosearch "modified \"$queue_name\""       ;# string we want to see
     set position [string last "$tosearch" "$elem"];# stores the position of $tosearch
         if { $position < 0 } {
            puts "Could not modify queue $queue_name."
            set return_value -1                    ;# if one operation fails, we return -1
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
   global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

   log_user 0 
	set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod -s $id"
	set sid [ open_spawn_process $program  ]     
   set sp_id [ lindex $sid 1 ]
	set timeout 30
   set result -1	
   log_user 0 

	expect {
      -i $sp_id full_buffer {
         set result -1 
         add_proc_error "suspend_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
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
  global CHECK_PRODUCT_ROOT CHECK_ARCH open_spawn_buffer

  log_user 0 
  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qmod -us $job"
  set sid [ open_spawn_process $program  ]     
  set sp_id [ lindex $sid 1 ]
  set timeout 30
  set result -1	
  log_user 0 

  expect {
       -i $sp_id full_buffer {
          set result -1 
          add_proc_error "unsuspend_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
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


#                                                             max. column:     |
#****** sge_procedures/delete_job() ******
# 
#  NAME
#     delete_job -- delete job with jobid
#
#  SYNOPSIS
#     delete_job { jobid } 
#
#  FUNCTION
#     This procedure will delete the job with the given jobid
#
#  INPUTS
#     jobid - job identification number
#
#  RESULT
#     0   - ok
#    -1   - timeout error
#
#  SEE ALSO
#     sge_procedures/submit_job()
#*******************************
proc delete_job { jobid } {
   global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer

   # spawn process
   log_user 0
   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qdel $jobid"
   set id [ open_spawn_process $program  ]
   set sp_id [ lindex $id 1 ]
   set result -1
   set timeout 30 	
   log_user 0 

   expect {
       -i $sp_id full_buffer {
          set result -1
          add_proc_error "delete_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
       }
       -i $sp_id "registered the job" {
          set result 0
       }
       -i $sp_id "has deleted job" {
          set result 0
       }
       -i $sp_id default {
          set result -1 
       }
       

   }
   # close spawned process 
   close_spawn_process $id 1
   log_user 1
   if { $result != 0 } {
      add_proc_error "delete_job" -1 "could not delete job $jobid"
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
#     submit_job { args {do_error_check 1} {submit_timeout 30} } 
#
#  FUNCTION
#     This procedure will submit a job.
#
#  INPUTS
#     args                - a string of qsub arguments/parameters
#     {do_error_check 1}  - if 1 (default): add global erros (add_proc_error)
#                           if not 1: do not add errors
#     {submit_timeout 30} - timeout (default is 30 sec.)
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
proc submit_job { args {do_error_check 1} {submit_timeout 30} } {
  global CHECK_PRODUCT_ROOT CHECK_ARCH CHECK_OUTPUT open_spawn_buffer

  set return_value " "

  # spawn process
  set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qsub $args"
  set id [ open_spawn_process $program  ]
  set sp_id [ lindex $id 1 ]

  set timeout $submit_timeout
  
  log_user 0

  expect {
       -i $sp_id full_buffer {
          set return_value -1    
          add_proc_error "submit_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
       }
       -i $sp_id timeout {
          set return_value -1 
       }
       -i $sp_id eof {
          set return_value -1
       }
       -i $sp_id "job*has been submitted" {
          
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
                set return_value -1 
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
       -i $sp_id -- "usage" {
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
       
     }

     # close spawned process 

     if { $do_error_check == 1 } {
        close_spawn_process $id
     } else {
        close_spawn_process $id 1
     }
   
     if {$do_error_check == 1} { 
       switch -- $return_value {
          "-1" { add_proc_error "submit_job" $return_value "timeout error" }
          "-2" { add_proc_error "submit_job" 0 "usage was printed on -help or commandfile argument - ok" }
          "-3" { add_proc_error "submit_job" $return_value "usage was printed NOT on -help or commandfile argument - error" }
          "-4" { add_proc_error "submit_job" 0 "verify output was printed on -verify argument - ok" }
          "-5" { add_proc_error "submit_job" $return_value "verify output was NOT printed on -verfiy argument - error" }
          "-6" { add_proc_error "submit_job" $return_value "job could not be scheduled, try later - error" }
          default { add_proc_error "submit_job" 0 "job $return_value submitted - ok" }
       }
     }
     return $return_value
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
      if { [lindex $line 0] == $jobid } {
         lappend back $line
      }
      if { $add_empty != 0 } {
         if { [llength $line] == 9 } {
            lappend back "-1 $line"
            puts $CHECK_OUTPUT "adding empty job lines" 
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
   global CHECK_PRODUCT_ROOT CHECK_ARCH
   upvar $variable qacctinfo
   
   set exit_code [catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qacct" -j $jobid} result]

   if { $exit_code == 0 } {
      parse_qacct result qacctinfo $jobid
      return 1
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

   set mytime [timestamp]

   if { $mytime == $check_timestamp } {
      sleep 1
   }
   set check_timestamp $mytime

   set catch_state [ catch { exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-f" } result ]

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




# wait for start of job ($jobid,$jobname) ; timeout after $seconds
# results : -1 on timeout ; 0 on jobstart
#                                                             max. column:     |
#****** sge_procedures/wait_for_jobstart() ******
# 
#  NAME
#     wait_for_jobstart -- wait for job to get out of pending list
#
#  SYNOPSIS
#     wait_for_jobstart { jobid jobname seconds {do_errorcheck 1} } 
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
proc wait_for_jobstart { jobid jobname seconds {do_errorcheck 1} } {
  
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for start of job $jobid ($jobname)"
   
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
    sleep 1
    set run_result [get_standard_job_info $jobid ]
    set run_result [ lindex $run_result 0 ]
    set state [lindex $run_result 4]
 
    if { [string first "t" $state ] < 0} {
       break;
    }
    set runtime [expr ( [timestamp] - $time) ]
    if { $runtime >= $seconds } {
       add_proc_error "wait_for_end_of_transfer" -1 "timeout waiting for job \"$jobid\""
       return -1
    }
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
#     wait_for_jobpending { jobid jobname seconds } 
#
#  FUNCTION
#     This procedure will return when the job is in pending state.
#
#  INPUTS
#     jobid   - job identification number
#     jobname - name of the job
#     seconds - timeout value in seconds
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
proc wait_for_jobpending { jobid jobname seconds} {
  
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for job $jobid ($jobname) to get in pending state"
  
  set time [timestamp] 
  while {1} {
    set run_result [is_job_running $jobid $jobname]
    if {$run_result == 0} {
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

   global CHECK_PRODUCT_ROOT CHECK_ARCH  open_spawn_buffer

   # spawn process
   log_user 0
   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qhold $jobid"
   set id [ open_spawn_process $program  ]
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

   global CHECK_PRODUCT_ROOT CHECK_ARCH  open_spawn_buffer
 
   # spawn process
   log_user 0
   set program "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qrls $jobid"
   set id [ open_spawn_process $program  ]
   set sp_id [ lindex $id 1 ]
   set timeout 30
   set result -1	
   log_user 0 

   expect {
       -i $sp_id full_buffer {
          set result -1
          add_proc_error "release_job" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
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
#     wait_for_jobend { jobid jobname seconds } 
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
proc wait_for_jobend { jobid jobname seconds {runcheck 1} } {
  
  global CHECK_OUTPUT

  puts $CHECK_OUTPUT "Waiting for end of job $jobid ($jobname)"
  
  if { $runcheck == 1 } {
     if { [is_job_running $jobid $jobname] != 1 } {
        add_proc_error "wait_for_jobend" -2 "job \"$jobid\" \"$jobname\" is not running"
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
       add_proc_error "wait_for_jobend" -1 "timeout waiting for job \"$jobid\" \"$jobname\""
       return -1
    }
    sleep 1
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
    
   if { [file isfile "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf"] == 1 } {
      catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qstat" "-help" } result
      set help [ split $result "\n" ] 
      if { ([ string first "fopen" [ lindex $help 0] ] >= 0) || 
           ([ string first "error" [ lindex $help 0] ] >= 0) } {
          set CHECK_PRODUCT_VERSION_NUMBER "system not running - run install test first"
          return $CHECK_PRODUCT_VERSION_NUMBER
      }
      set CHECK_PRODUCT_VERSION_NUMBER [ lindex $help 0]
      if { [ string first "exit" $CHECK_PRODUCT_VERSION_NUMBER ] >= 0 } {
         set CHECK_PRODUCT_VERSION_NUMBER "system not running - run install test first"
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
   global CHECK_START_SCRIPT_NAME

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_qmaster" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   } 

   puts $CHECK_OUTPUT "starting up qmaster on host \"$CHECK_CORE_MASTER\" as user \"$startup_user\""

   set output [start_remote_prog "$CHECK_CORE_MASTER" "$startup_user" "$CHECK_PRODUCT_ROOT/default/common/$CHECK_START_SCRIPT_NAME" "-qmaster"]
   if { [string first "found running qmaster with pid" $output] >= 0 } {
      add_proc_error "startup_qmaster" -1 "qmaster on host $CHECK_CORE_MASTER is allready running"
      return -1
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
   global CHECK_START_SCRIPT_NAME

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

   if { [string first "execd is already running" $output] >= 0 } {
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

   if { [string first "starting sge_shadowd" $output] >= 0 } {
       return 0
   }
   add_proc_error "startup_shadowd" -1 "could not start shadowd on host $hostname"
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
proc shutdown_master_and_scheduler { hostname qmaster_spool_dir} {

   global CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_ROOT CHECK_ADMIN_USER_SYSTEM
   global CHECK_PRODUCT_TYPE CHECK_ARCH

   puts $CHECK_OUTPUT "shutdown_master_and_scheduler ..."

   puts $CHECK_OUTPUT ""
   puts $CHECK_OUTPUT "killing qmaster and scheduler on host $hostname ..."
   puts $CHECK_OUTPUT "retrieving data from spool dir $qmaster_spool_dir"



   set qmaster_pid -1
   set scheduler_pid -1

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


   get_ps_info $scheduler_pid $hostname
   if { ($ps_info($scheduler_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $scheduler_pid "sge_schedd" ] == 0 } { 
         puts $CHECK_OUTPUT "killing schedd with pid $scheduler_pid on host $hostname"

         catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ks" } result

         shutdown_system_daemon $hostname sched

      } else {
         add_proc_error "shutdown_master_and_scheduler" "-1" "scheduler pid $scheduler_pid not found"
         set scheduler_pid -1
      }
   } else {
      add_proc_error "shutdown_master_and_scheduler" "-1" "ps_info failed, pid=$scheduler_pid"
      set scheduler_pid -1
   }

   get_ps_info $qmaster_pid $hostname
   if { ($ps_info($qmaster_pid,error) == 0) } {
      if { [ is_pid_with_name_existing $hostname $qmaster_pid "sge_qmaster" ] == 0 } { 

         puts $CHECK_OUTPUT "killing qmaster with pid $qmaster_pid on host $hostname"

         catch {  eval exec "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-km" } result

         shutdown_system_daemon $hostname qmaster

      } else {
         add_proc_error "shutdown_master_and_scheduler" "-1" "qmaster pid $qmaster_pid not found"
         set qmaster_pid -1
      }
   } else {
      add_proc_error "shutdown_master_and_scheduler" "-1" "ps_info failed, pid=$qmaster_pid"
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
global CHECK_HOST CHECK_ARCH CHECK_PRODUCT_ROOT CHECK_PRODUCT_TYPE CHECK_CORE_EXECD 
global CHECK_CORE_MASTER CHECK_CORE_INSTALLED CHECK_OUTPUT CHECK_USER CHECK_PRODUCT_TYPE
global CHECK_COMMD_PORT CHECK_ADMIN_USER_SYSTEM do_compile

   puts $CHECK_OUTPUT "killing qmaster, scheduler and all execds in the cluster ..."

   set result ""
   set do_ps_kill 0
   set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/qconf" "-ke -ks -km" ]

   puts $CHECK_OUTPUT "qconf -ke -ks -km returned $prg_exit_state"
   if { $prg_exit_state == 0 } {
      puts $CHECK_OUTPUT $result
   } else {
      set do_ps_kill 1
      puts $CHECK_OUTPUT "shutdown_core_system - qconf error or binary not found\n$result"
   }

   sleep 5  ;# give the qmaster time
   puts $CHECK_OUTPUT "killing all commds in the cluster ..." 

   foreach elem $CHECK_CORE_EXECD { 
       puts $CHECK_OUTPUT "killing commd on host $elem"

       if { $CHECK_COMMD_PORT < 1024 } {
         if { [ have_root_passwd ] == -1 } {
            set_root_passwd 
         }
         set result [ start_remote_prog "$CHECK_CORE_MASTER" "root" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/sgecommdcntl" "-k -host $elem"  ]
       } else {
         set result [ start_remote_prog "$CHECK_CORE_MASTER" "$CHECK_USER" "$CHECK_PRODUCT_ROOT/bin/$CHECK_ARCH/sgecommdcntl" "-k -host $elem"  ]
       }
       if { $prg_exit_state == 0 } {
          puts $CHECK_OUTPUT $result
       } else {
          set do_ps_kill 1
          puts $CHECK_OUTPUT "shutdown_core_system - commdcntl error or binary not found"
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
  global CHECK_PRODUCT_ROOT CHECK_ARCH  CHECK_OUTPUT

  set catch_return [ catch { exec "$CHECK_PRODUCT_ROOT/utilbin/$CHECK_ARCH/gethostname" "-name"} result ]
  if { $catch_return == 0 } {
     set result [split $result "."]
     set newname [lindex $result 0]
     return $newname
  } else {
     puts $CHECK_OUTPUT "proc gethostname - gethostname error or binary not found"
     puts $CHECK_OUTPUT "error: $result"
     puts $CHECK_OUTPUT "error: $catch_return"
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
  global CHECK_SCRIPT_FILE_DIR CHECK_USER


  if { [ info exists arch_cache($host) ] } {
     return $arch_cache($host)
  }

  if { [ info exists CHECK_USER ] == 0 } {
     puts $CHECK_OUTPUT "user not set, aborting"
     return "unknown"
  }

  if { [ string compare $host "none" ] == 0 } {
      set prg_exit_state [ catch { eval exec "$CHECK_PRODUCT_ROOT/util/arch" } result ]
  } else {
      set result [ start_remote_prog $host $CHECK_USER "$CHECK_PRODUCT_ROOT/util/arch" "" ]
  }

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
     puts $CHECK_OUTPUT "architecture or file \"$CHECK_PRODUCT_ROOT/util/arch\" not found"
     return "unknown"
  }
  set result [lindex $result 0]  ;# remove CR

  if { [ string compare $result "" ] == 0 } {
     puts $CHECK_OUTPUT "architecture or file \"$CHECK_PRODUCT_ROOT/util/arch\" not found"
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

  set result [ start_remote_prog $host $CHECK_USER "$CHECK_SOURCE_DIR/c4/aimk" "-nomk" ]
 
  set result [split $result "\n"]
  set result [join $result ""]
  set result [split $result "\r"]
  set result [join $result ""]

  if { $prg_exit_state != 0 } {
     add_proc_error "resolve_upper_arch" "-1" "architecture not found or aimk not found in $CHECK_SOURCE_DIR/c4"
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

  set result [ start_remote_prog $name $CHECK_USER "$CHECK_PRODUCT_ROOT/utilbin/$remote_arch/gethostname" "-name" ]
  set result [ lindex $result 0 ]  ;# removing /r /n

  if { $prg_exit_state != 0 } {
     puts $CHECK_OUTPUT "proc reslove_host - gethostname error or file \"$CHECK_PRODUCT_ROOT/utilbin/$remote_arch/gethostname\" not found"
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

