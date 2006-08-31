#!/usr/local/bin/tclsh
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

# JG: TODO: Change the assign/unassign procedures.
# The current implemtation using aattr/dattr is destroying the default 
# settings in all.q

proc get_complex { change_array } {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sc" } result ]
  if { $catch_result != 0 } {
     add_proc_error "get_complex" "-1" "qconf error or binary not found ($ts_config(product_root)/bin/$CHECK_ARCH/qconf)\n$result"
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

#****** sge_procedures.60/set_complex() **********************************
#  NAME
#     set_complex() -- set complexes with the qconf -mc commaned
#
#  SYNOPSIS
#     set_complex { change_array } 
#
#  FUNCTION
#     Modifies, adds or deletes complexes
#
#     If an complex in change_array already exits the complex will be changed
#     If it not exists in will be added
#     If the complex definition in the change_array is a empty string the
#     complex will be deleted
#
#  INPUTS
#     change_array - array with the complex definitions
#
#  RETURN:
#
#       -1  complex definition has been modified
#       -2  complex definition has been added
#       -3  complex definition has been removed
#       -4  complex definition has not changed
#     else  error
#
#  EXAMPLE:
#
#  1. add or modify a complexes
#
#      set tmp_complex(slots) "s   INT <= YES YES 1 1000"
#      set tmp_complex(dummy) "du1 INT <= YES YES 0 500"
#     
#      set_complex tmp_complex
#
#   2. delete a complex
#
#      set tmp_complex(dummy) ""
#      set_complex tmp_complex
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc set_complex { change_array } {
  global ts_config CHECK_USER
  global env CHECK_ARCH CHECK_OUTPUT
  global CHECK_CORE_MASTER
  upvar $change_array chgar
  set values [array names chgar]

  get_complex old_values

#  set names [array names old_values]
#  foreach name $names {
#     puts $CHECK_OUTPUT "$name = $old_values($name)"
#  }

  set vi_commands {}
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal $chgar($elem)
     if {[info exists old_values($elem)]} {
        # if old and new config have the same value, create no vi command,
        # if they differ, add vi command to ...
        if { [compare_complex $old_values($elem) $newVal] != 0 } {
           if { $newVal == "" } {
              # ... delete config entry (replace by comment)
              lappend vi_commands ":%s/^$elem .*$/#/\n"
           } else {
              # ... change config entry
              set newVal1 [split $newVal {/}]
              set newVal [join $newVal1 {\/}]
              lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
           }
        }
     } else {
        # if the config entry didn't exist in old config: append a new line
        lappend vi_commands "A\n$elem  $newVal[format "%c" 27]"
     }
  }

#  foreach vi_com $vi_commands {
#     puts $CHECK_OUTPUT "\"$vi_com\""
#  }

  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ADDED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set NOT_MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_CENTRY_NOTCHANGED]]
  
  set result [ handle_vi_edit "echo" "\"\"\nSGE_ENABLE_MSG_ID=1\nexport SGE_ENABLE_MSG_ID\n$ts_config(product_root)/bin/$CHECK_ARCH/qconf -mc" $vi_commands $MODIFIED $REMOVED $ADDED $NOT_MODIFIED ]
  if { $result != 0 && $result != -2 && $result != -3 && $result != -4 } {
     add_proc_error "set_complex" -1 "could not modify complex: ($result)"
  }
  return $result
}



#****** sge_procedures.60/switch_to_admin_user_system() ************************
#  NAME
#     switch_to_admin_user_system() -- switch to a admin user system
#
#  SYNOPSIS
#     switch_to_admin_user_system { } 
#
#  FUNCTION
#     run install core system and install admin user system
#
#  INPUTS
#
#  RESULT
#     0 - on success
#
#  NOTES
#     not implemented
#
#  SEE ALSO
#     sge_procedures.60/switch_to_admin_user_system()
#     sge_procedures.60/switch_to_normal_user_system()
#     sge_procedures.60/switch_to_root_user_system()
#*******************************************************************************
proc switch_to_admin_user_system {} {
   global CHECK_OUTPUT actual_user_system

   if { $actual_user_system != "admin user system" } {
      puts $CHECK_OUTPUT "switching from $actual_user_system to admin user system ..."
      add_proc_error "switch_to_admin_user_system" -3 "Function not implemented"
      set actual_user_system "admin user system"
   }

   return 0
}

#****** sge_procedures.60/switch_to_root_user_system() *************************
#  NAME
#     switch_to_root_user_system() -- switch to a root user system
#
#  SYNOPSIS
#     switch_to_root_user_system { } 
#
#  FUNCTION
#     run install core system and install root user system
#
#  INPUTS
#
#  RESULT
#     0 - on success
#
#  NOTES
#     not implemented
#
#  SEE ALSO
#     sge_procedures.60/switch_to_admin_user_system()
#     sge_procedures.60/switch_to_normal_user_system()
#     sge_procedures.60/switch_to_root_user_system()
#*******************************************************************************
proc switch_to_root_user_system {} {
   global CHECK_OUTPUT actual_user_system
    
   add_proc_error "switch_to_root_user_system" -3 "Function not implemented"
   return 1

   if { $actual_user_system != "root user system" } {
      puts $CHECK_OUTPUT "switching from $actual_user_system to root user system ..."
      set actual_user_system "root user system"
   }
}

#****** sge_procedures.60/switch_to_normal_user_system() ***********************
#  NAME
#     switch_to_normal_user_system() -- switch to a standard user system
#
#  SYNOPSIS
#     switch_to_normal_user_system { } 
#
#  FUNCTION
#      run install core system and install standard user system
#
#  INPUTS
#
#  RESULT
#     0 - on success
#
#  NOTES
#     not implemented
#
#  SEE ALSO
#     sge_procedures.60/switch_to_admin_user_system()
#     sge_procedures.60/switch_to_normal_user_system()
#     sge_procedures.60/switch_to_root_user_system()
#*******************************************************************************
proc switch_to_normal_user_system {} {
   global CHECK_OUTPUT actual_user_system

   add_proc_error "switch_to_root_user_system" -3 "Function not implemented"
   return 1

   if { $actual_user_system != "normal user system" } {
      puts $CHECK_OUTPUT "switching from $actual_user_system to normal user system ..."
      set actual_user_system "normal user system"
   }
}

#****** sge_procedures.60/switch_execd_spool_dir() *****************************
#  NAME
#     switch_execd_spool_dir() -- switch execd spool directory
#
#  SYNOPSIS
#     switch_execd_spool_dir { host spool_type { force_restart 0 } } 
#
#  FUNCTION
#     This function will shutdown the execd running on $host, switch the
#     spool type depending on $spool_type if the spool directory doesn't
#     match. The optional parameter force_restart can be used to 
#     shutdown/restart the execd even when the spool directory is already
#     set to the correct value.
#
#  INPUTS
#     host                - host of execd
#     spool_type          - "cell", "local", "NFS-ROOT2NOBODY" or "NFS-ROOT2ROOT"
#     { force_restart 0 } - optional if 1: do shutdown/restart even when
#                           spool directory is already matching
#
#  RESULT
#     0 - on success
#
#  SEE ALSO
#     file_procedures/get_execd_spooldir()
#*******************************************************************************
proc switch_execd_spool_dir { host spool_type { force_restart 0 } } {
   global CHECK_OUTPUT

   set spool_dir [get_execd_spooldir $host $spool_type]
   set base_spool_dir [get_execd_spooldir $host $spool_type 1]

   if { [info exists execd_config] } {
      unset execd_config
   }
   if { [get_config execd_config $host] != 0 } {
      add_proc_error "switch_execd_spool_dir" -1 "can't get configuration for host $host"
      return -1
   }

   if { $execd_config(execd_spool_dir) == $spool_dir && $force_restart == 0 } {
      debug_puts "spool dir is already set to $spool_dir"
      return 0
   }
   
   puts $CHECK_OUTPUT "$host: actual spool dir: $execd_config(execd_spool_dir)"
   puts $CHECK_OUTPUT "$host: new spool dir   : $spool_dir"
 
   delete_all_jobs
   wait_for_end_of_all_jobs 60

   shutdown_system_daemon $host execd

   puts $CHECK_OUTPUT "changing execd_spool_dir for host $host ..."
   set execd_config(execd_spool_dir) $spool_dir
   set_config execd_config $host

   if { [ remote_file_isdirectory $host $base_spool_dir ] != 1 } {
      puts $CHECK_OUTPUT "creating not existing base spool directory:\n\"$base_spool_dir\""
      remote_file_mkdir $host $base_spool_dir
   }   

   puts $CHECK_OUTPUT "cleaning up spool dir $spool_dir ..."
   cleanup_spool_dir_for_host $host $base_spool_dir "execd"
   

   startup_execd $host

   wait_for_load_from_all_queues 100

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
#*******************************
proc startup_shadowd { hostname {env_list ""} } {
   global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER

   if {$env_list != ""} {
      upvar $env_list envlist
   }

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

   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/sgemaster" "-shadowd start" prg_exit_state 60 0 envlist]
   puts $CHECK_OUTPUT $output
   if { [string first "starting sge_shadowd" $output] >= 0 } {
       return 0
   }
   add_proc_error "startup_shadowd" -1 "could not start shadowd on host $hostname:\noutput:\"$output\""
   return -1
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
proc startup_execd { hostname {envlist ""}} {
   global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER

   upvar $envlist my_envlist

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
   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/sgeexecd" "start" prg_exit_state 60 0 my_envlist 1 1 1]

   return 0
}

#                                                             max. column:     |
#****** sge_procedures/startup_bdb_rpc() ******
# 
#  NAME
#     startup_bdb_rpc -- ??? 
#
#  SYNOPSIS
#     startup_bdb_rpc { hostname } 
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
#     sge_procedures/startup_bdb_rpc()
#*******************************
proc startup_bdb_rpc { hostname } {
  global ts_config
   global CHECK_OUTPUT
   global CHECK_ADMIN_USER_SYSTEM CHECK_USER

   if { $hostname == "none" } {
      return -1
   }

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } {  
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_bdb_rpc" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }
 

   puts $CHECK_OUTPUT "starting up BDB RPC Server on host \"$hostname\" as user \"$startup_user\""

   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/sgebdb" "start"]
   puts $CHECK_OUTPUT $output
   if { [string length $output] < 15  && $prg_exit_state == 0 } {
       return 0
   }
   add_proc_error "startup_bdb_rpc" -1 "could not start berkeley_db_svc on host $hostname:\noutput:\"$output\""
   return -1
}

#                                                             max. column:     |
#****** sge_procedures/get_urgency_job_info() ******
# 
#  NAME
#     get_urgency_job_info -- get urgency job information (qstat -urg)
#
#  SYNOPSIS
#     get_urgency_job_info { jobid {variable job_info} } 
#
#  FUNCTION
#     This procedure is calling the qstat (qstat -urg if sgeee) and returns
#     the output of the qstat in array form.
#
#  INPUTS
#     jobid               - job identifaction number
#     {variable job_info} - name of variable array to store the output
#     {do_replace_NA}     - 1 : if not set, don't replace NA settings
#
#  RESULT
#     0, if job was not found
#     1, if job was found
#     
#     fills array $variable with info found in qstat output with the following symbolic names:
#
#     job-ID prior nurg urg rrcontr wtcontr  dlcontr name  user state submit/start at
#     deadline queue slots ja-task-ID 

#
#  EXAMPLE
#  proc testproc ... { 
#     ...
#     if {[get_urgency_job_info $job_id] } {
#        if { $job_info(urg) < 10 } {
#           ...
#        }
#     } else {
#        add_proc_error "testproc" -1 "get_urgency_job_info failed for job $job_id on host $host"
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
proc get_urgency_job_info {jobid {variable job_info} { do_replace_NA 1 } } {
  global ts_config
   global CHECK_ARCH
   upvar $variable jobinfo

   set exit_code [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" "-urg"} result]

   if { $exit_code == 0 } {
      parse_qstat result jobinfo $jobid 2 $do_replace_NA
      return 1
   }
  
   return 0
}

# ADOC see sge_procedures/get_sge_error_generic()
proc get_sge_error_generic_vdep {messages_var} {
   upvar $messages_var messages

   lappend messages(index) "-100"
   set messages(-100) "*[translate_macro MSG_GDI_UNABLE_TO_CONNECT_SUS "qmaster" "*" "*"]"
   set messages(-100,description) "probably sge_qmaster is down"
}
