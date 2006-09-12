#                                                             max. column:     |
#****** install_core_system/kill_running_system() ******
# 
#  NAME
#     kill_running_system -- shutdown an already running system
#
#  SYNOPSIS
#     kill_running_system { } 
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
proc kill_running_system {} {
   global ts_config
   global CHECK_ARCH 
   global CHECK_OUTPUT CORE_INSTALLED
   global check_use_installed_system
 
   set result [check_all_system_times]
   puts $CHECK_OUTPUT "check_all_system_times returned $result"
   if { $result != 0 } {
      add_proc_error "kill_running_system" -2 "skipping install_core_system"
      set_error 0 "ok"
      return
   }

   set CORE_INSTALLED ""
   write_install_list

   shutdown_core_system

   set_error "0" "kill_running_system - this is not a 'real' test (only try to shutdown $ts_config(product_type) system)"

   if { $check_use_installed_system == 0 } { 
      # if this is master remove default dir (but not the licence file)
   
         set moved [timestamp]
   
         catch { exec "mv" "$ts_config(product_root)/$ts_config(cell)" "$ts_config(product_root)/$ts_config(cell).$moved"  } result
         puts $result
   
         puts "current default directory ($ts_config(product_root)/$ts_config(cell)) was moved to default.$moved"
   
         sleep 2
   
         catch { exec "mkdir" "$ts_config(product_root)/$ts_config(cell)"  } result
         puts $result
         catch { exec "chmod" "755" "$ts_config(product_root)/$ts_config(cell)"  } result
         puts $result

   
         sleep 2
   
         catch { exec "mkdir" "$ts_config(product_root)/$ts_config(cell)/common"  } result
         puts $result

         catch { exec "chmod" "755" "$ts_config(product_root)/$ts_config(cell)/common"  } result
         puts $result
   
         catch { exec "cp" "$ts_config(product_root)/$ts_config(cell).$moved/common/host_aliases" "$ts_config(product_root)/$ts_config(cell)/common/host_aliases"  } result
         puts $result
  
         if { [file exists "$ts_config(product_root)/$ts_config(cell).$moved"] } {
            delete_directory "$ts_config(product_root)/$ts_config(cell).$moved"
         }
   }
}




proc reread_bootstrap {} {
  # install_qmaster has written the bootstrap or the configuration file
  bootstrap_sge_config
  set_error "0" "install_qmaster - no errors"
}


# generating all testsuite cluster user keys and certificates
proc make_user_cert {} {
   global ts_config 
   global check_use_installed_system
  global CHECK_OUTPUT CHECK_MAIN_RESULTS_DIR
  global CHECK_FIRST_FOREIGN_SYSTEM_USER CHECK_SECOND_FOREIGN_SYSTEM_USER CHECK_REPORT_EMAIL_TO
  global CHECK_USER CHECK_DEBUG_LEVEL

   if { !$check_use_installed_system } {
      if { $ts_config(product_feature) == "csp" } {
         puts $CHECK_OUTPUT "removing poss. existing user_file.txt \"$CHECK_MAIN_RESULTS_DIR/user_file.txt\" ..."
         set result [ start_remote_prog "$ts_config(master_host)" "$CHECK_USER" "rm" "$CHECK_MAIN_RESULTS_DIR/user_file.txt" ]
         puts $CHECK_OUTPUT $result
    
         puts $CHECK_OUTPUT "creating file \"$CHECK_MAIN_RESULTS_DIR/user_file.txt\" ..."
         set script [ open "$CHECK_MAIN_RESULTS_DIR/user_file.txt" "w" ]
         puts $script "$CHECK_FIRST_FOREIGN_SYSTEM_USER:first_testsuite_user:$CHECK_REPORT_EMAIL_TO"
         puts $script "$CHECK_SECOND_FOREIGN_SYSTEM_USER:second_testsuite_user:$CHECK_REPORT_EMAIL_TO"
         flush $script
         close $script
        
         set result [ start_remote_prog "$ts_config(master_host)" "root" "cd" "$ts_config(product_root) ; util/sgeCA/sge_ca -usercert $CHECK_MAIN_RESULTS_DIR/user_file.txt" ]
         puts $CHECK_OUTPUT $result
      
         puts $CHECK_OUTPUT "removing poss. existing user_file.txt \"$CHECK_MAIN_RESULTS_DIR/user_file.txt\" ..."
         set result [ start_remote_prog "$ts_config(master_host)" "$CHECK_USER" "rm" "$CHECK_MAIN_RESULTS_DIR/user_file.txt" ]
         puts $CHECK_OUTPUT $result
     } else {
        puts $CHECK_OUTPUT "no csp feature enabled"
     }
   }

   set_error 0 "ok"
}


proc cleanup_system { } {
   global ts_config
   global CHECK_ARCH env check_errno
   global check_use_installed_system CHECK_OUTPUT CHECK_USER

#puts "press RETURN"
#set anykey [wait_for_enter 1]
   # check if the system is running and qmaster accessable
   set catch_return [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" } result ]
   if { $catch_return != 0 } {
      add_proc_error "cleanup_system" -2 "error connecting qmaster: $result"
      set_error -2 "error connecting qmaster"
      return
   }
    
   puts $CHECK_OUTPUT "\ncleaning up system"

   # delete all jobs
   delete_all_jobs

   # wait until cluster is empty
   set my_time [ expr ( [timestamp] + 100 ) ]
   while { [timestamp]< $my_time } {
      set my_jobs [ get_standard_job_info 1 0 1 ]
      foreach job_elem $my_jobs {
         puts $CHECK_OUTPUT $job_elem
      }
      if { [llength $my_jobs] <= 2 } {
         break
      }
      sleep 1
   } 

   # SGEEE: remove sharetree
   if {[string compare $ts_config(product_type) "sgeee"] == 0} {
      del_sharetree
   }

   # remove all checkpoint environments
  puts $CHECK_OUTPUT "\nremoving ckpt objects ..."
  set NO_CKPT_INTERFACE_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "ckpt interface definition"]
   
  catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sckptl" } result
  if { [string first $NO_CKPT_INTERFACE_DEFINED $result] >= 0 } {
     puts $CHECK_OUTPUT "no ckpt interface definition defined"
  } else {
     foreach elem $result {
        puts $CHECK_OUTPUT "removing ckpt interface $elem."
        del_checkpointobj $elem 
     }
  }

   # remove all parallel environments
  puts $CHECK_OUTPUT "\nremoving PE objects ..."
  set NO_PARALLEL_ENVIRONMENT_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "parallel environment"]
  catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-spl" } result
  if { [string first $NO_PARALLEL_ENVIRONMENT_DEFINED $result] >= 0 } {
     puts $CHECK_OUTPUT "no parallel environment defined"
  } else {
     foreach elem $result {
        puts $CHECK_OUTPUT "removing PE $elem."
        del_pe $elem 
     }
  }
 
   # remove all calendars
  puts $CHECK_OUTPUT "\nremoving calendars ..."
  # JG: TODO: calendars can be referenced in queues - first remove all references!
  set NO_CALENDAR_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "calendar"]
  catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-scall" } result
  if { [string first $NO_CALENDAR_DEFINED $result] >= 0 } {
     puts $CHECK_OUTPUT "no calendar defined"
  } else {
     foreach elem $result {
        puts $CHECK_OUTPUT "removing calendar $elem."
        del_calendar $elem 
     }
  }

   # remove all projects 
  if { [ string compare $ts_config(product_type) "sgeee" ] == 0 } {
     puts $CHECK_OUTPUT "\nremoving project objects ..."
     set NO_PROJECT_LIST_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "project list"]
     catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sprjl" } result
     if { [string first $NO_PROJECT_LIST_DEFINED $result] >= 0 } {
        puts $CHECK_OUTPUT "no project list defined"
     } else {
        foreach elem $result {
           puts $CHECK_OUTPUT "removing project $elem."
           del_prj $elem 
        }
     }
  }

   # JG: TODO: what about SGEEE users?
  
   # remove all access lists
   # JG: TODO: accesslists are referenced in a variety of objects - first delete them
   #           there!
  puts $CHECK_OUTPUT "\nremoving access lists ..."
  set NO_ACCESS_LIST_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "userset list"]
  catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sul" } result
  if { [string first $NO_ACCESS_LIST_DEFINED $result] >= 0 } {
     puts $CHECK_OUTPUT "no userset list defined"
  } else {
     foreach elem $result {
        if { [ string compare $elem defaultdepartment ] == 0 } {
           puts $CHECK_OUTPUT "skipping \"defaultdepartment\" ..."
           continue
        }
        puts $CHECK_OUTPUT "removing userset list $elem."
        del_access_list $elem
     }
  }

   # remove all queues
   puts $CHECK_OUTPUT "\nremoving queues ..."
   set queue_list [get_queue_list]
   foreach elem $queue_list {
      puts $CHECK_OUTPUT "removing queue $elem."
      del_queue $elem "" 1 1
   }

   # cleanup the tmpdir's referenced in queues
   cleanup_tmpdirs 

   # add new testsuite queues
  puts $CHECK_OUTPUT "\nadding testsuite queues ..."
  add_queue "all.q" "@allhosts" q_param 1
  
  
  # execute the clean hooks of all checktrees
  if { [ exec_checktree_clean_hooks ] != 0 } {
     set_error 1 "exec_checktree_clean_hooks reported an error"
  }
  
  
  set_error 0 "ok"
}


#                                                             max. column:     |
#****** install_core_system/setup_queues() ******
# 
#  NAME
#     setup_queues -- ??? 
#
#  SYNOPSIS
#     setup_queues { } 
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
proc setup_queues {} {
   global ts_config
   global CHECK_ARCH env check_errno
   global check_use_installed_system CHECK_OUTPUT CHECK_USER

   # check if qmaster can be accessed
   set catch_return [ catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat" } result ]
   if { $catch_return != 0 } {
      add_proc_error "setup_queues" -2 "error connecting qmaster: $result"
      set_error -2 "error connecting qmaster"
      return
   }

   # for all queues: set load_thresholds and queue type
   set new_values(load_thresholds)       "np_load_avg=11.00"
   set new_values(qtype)                 "BATCH INTERACTIVE CHECKPOINTING PARALLEL"
 
   set result [mod_queue "all.q" "" new_values]
   switch -- $result { 
      -1 {
         set_error -1 "setup_queues - modify queue ${hostname}.q - got timeout"
      }
      0 {
         set_error 0 "setup_queues - no errors"
      }
      -100 {
         set_error -1 "setup_queues - could not modify queue"
      } 
   }

   if { $check_errno == 0} {
      # for each individual queue set the slots attribute
      foreach hostname $ts_config(execd_nodes) {
         unset new_values
         set index [lsearch $ts_config(execd_nodes) $hostname] 
         set slots_tmp [node_get_processors $hostname]

         if { $slots_tmp <= 0 } {
            set_error -2 "no slots for execd $hostname"
            return
         }

         set slots [ expr ( $slots_tmp * 10) ]
         set new_values(slots) "$slots"

         set result [mod_queue "all.q" $hostname new_values]
         switch -- $result { 
            -1 {
               set_error -1 "setup_queues - modify queue ${hostname}.q - got timeout"
            }
            0 {
               set_error 0 "setup_queues - no errors"
            }
            -100 {
               set_error -1 "setup_queues - could not modify queue"
            } 
         }
      }
   }

   # wait until all hosts are up
   if { $check_errno == 0 } {
      wait_for_load_from_all_queues 300 
      set_error 0 "ok"
   }
}

#                                                             max. column:     |
#****** install_core_system/setup_testcheckpointobject() ******
# 
#  NAME
#     setup_testcheckpointobject -- ??? 
#
#  SYNOPSIS
#     setup_testcheckpointobject { } 
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
proc setup_testcheckpointobject {} {
   global ts_config

   set change(ckpt_name)  "testcheckpointobject"
   add_checkpointobj change
   assign_queues_with_ckpt_object "all.q" "" "testcheckpointobject"

   set_error 0 "ok"
}

proc setup_limitationrulesobjects {} {
   global ts_config

   # limitation rule set max_jobs
   set lirs_name "max_jobs"
   set rules ""
   lappend rules "to slots=30000"

   set lirs_charray($lirs_name,limit) "$rules"
   set lirs_charray($lirs_name,enabled) "true"
   add_lirs lirs_charray

   # limitation rule set per_host
   set lirs_name "per_host"
   set rules ""
   lappend rules "hosts `*` to slots=30000"
   set lirs_charray($lirs_name,limit) "$rules"
   set lirs_charray($lirs_name,enabled) "true"
   add_lirs lirs_charray

   # limitation rule set per_host
   set lirs_name "per_queue"
   set rules ""
   lappend rules "queues `*` to slots=30000"
   set lirs_charray($lirs_name,limit) "$rules"
   set lirs_charray($lirs_name,enabled) "true"
   add_lirs lirs_charray

   set_error 0 "ok"
}

#                                                             max. column:     |
#****** install_core_system/setup_conf() ******
# 
#  NAME
#     setup_conf -- ??? 
#
#  SYNOPSIS
#     setup_conf { } 
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
proc setup_conf {} {
   global ts_config
  global CHECK_ARCH
  global CHECK_DEFAULT_DOMAIN 
  global CHECK_REPORT_EMAIL_TO 
  global CHECK_USER 
  global CHECK_DNS_DOMAINNAME

  get_config old_config
  set params(reschedule_unknown) "00:00:00"
  set_config params
  get_config old_config

  # set finished_job in global config
  set params(finished_jobs) "0"  
  set params(load_report_time) "00:00:15"
  set params(reschedule_unknown) "00:00:00"
  set params(loglevel) "log_info"
  set params(load_sensor) "none"
  set params(prolog) "none"
  set params(epilog) "none"
  set params(shell_start_mode) "posix_compliant"
  set params(login_shells) "sh,ksh,csh,tcsh"
  set params(min_uid) "0"
  set params(min_gid) "0"
  set params(user_lists) "none"
  set params(xuser_lists) "none"
  set params(max_unheard) "00:01:00"

  if { $CHECK_REPORT_EMAIL_TO == "none" } {
     set params(administrator_mail) "${CHECK_USER}@${CHECK_DNS_DOMAINNAME}"
  } else {
     set params(administrator_mail) $CHECK_REPORT_EMAIL_TO
  }

  set params(set_token_cmd) "none"
  set params(pag_cmd) "none"
  set params(token_extend_time) "none"
  set params(shepherd_cmd) "none"
  set params(qmaster_params) "none"
  if { $ts_config(gridengine_version) == 53 } {
     set params(schedd_params) "none"
  } else {
     set params(reporting_params) "accounting=true reporting=false flush_time=00:00:05 joblog=true sharelog=00:10:00"
  }
  set params(execd_params) "none"
  set params(qlogin_command) "telnet"
  set params(max_aj_instances) "2000"
  set params(max_aj_tasks) "75000"

  # default domain and ignore_fqdn have moved to the bootstrap file
  # in SGE 6.0. Only check them in older systems.
  if [info exists old_config(default_domain)] {
    set params(default_domain) "$CHECK_DEFAULT_DOMAIN"
  }
  if [info exists old_config(ignore_fqdn)] {
    set params(ignore_fqdn) "true"
  }

  if { $ts_config(product_type) == "sgeee" } {
    set params(execd_params)    "PTF_MIN_PRIORITY=20,PTF_MAX_PRIORITY=0,SET_LIB_PATH=true"
    set params(enforce_project) "false"
    set params(projects) "none"
    set params(xprojects) "none"
  }

  set result [ set_config params ] 

  # for sgeee systems on irix: set execd params to
  # PTF_MIN_PRIORITY=40,PTF_MAX_PRIORITY=20,SET_LIB_PATH=true
  if { $ts_config(product_type) == "sgeee" } {
  set ptf_param(execd_params) "PTF_MIN_PRIORITY=40,PTF_MAX_PRIORITY=20,SET_LIB_PATH=true"
    foreach i $ts_config(execd_nodes) {
      switch -exact [resolve_arch $i] {
        irix6 {
          set_config ptf_param $i
        }
      }
    }
  }

  get_config new_config
  
  set arrays_old [ array names old_config ]
  set arrays_new [ array names new_config ]

  if { [ llength $arrays_old] == [ llength $arrays_new ] } {
    foreach param $arrays_old {
       set old  $old_config($param)
       set new  $new_config($param)

       if { [ string compare -nocase $old $new ] != 0 } {
          if { [ string compare $param "load_report_time" ] == 0 } { continue }
          if { [ string compare $param "loglevel" ] == 0 } { continue }
          if { [ string compare $param "execd_params" ] == 0 } { continue }
          if { [ string compare $param "finished_jobs" ] == 0 } { continue }
          if { [ string compare $param "max_unheard" ] == 0 } { continue }
          if { [ string compare $param "reporting_params" ] == 0 } { continue }

          add_proc_error "setup_conf" -3 "config parameter $param:\ndefault setup: $old, after testsuite reset: $new" 
       }
    }
  } else {
      foreach elem $arrays_old {
         if { [string first $elem $arrays_new] < 0 } {
            add_proc_error "setup_conf" -1 "paramter $elem not in new configuration"
         }
      }
      foreach elem $arrays_new { 
         if { [string first $elem $arrays_old] < 0 } {
           add_proc_error "setup_conf" -1 "paramter $elem not in old configuration"
         }
      }

     add_proc_error "setup_conf" -1 "config parameter count new/old configuration error"
  }

  set_error 0 "ok"
}

#****** check/setup_execd_conf() ***********************************************
#  NAME
#     setup_execd_conf() -- ??? 
#
#  SYNOPSIS
#     setup_execd_conf { } 
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
proc setup_execd_conf {} {
   global ts_config
  global CHECK_ARCH
  global CHECK_DEFAULT_DOMAIN
  global CHECK_OUTPUT

  foreach host $ts_config(execd_nodes) {
     puts $CHECK_OUTPUT "get configuration for host $host ..."
     if [info exists tmp_config] {
        unset tmp_config
     }
     get_config tmp_config $host
     if { [info exists elements] } {
        unset elements
     }

     if {! [info exists tmp_config]} {
        add_proc_error "setup_execd_conf" -1 "couldn't get conf - skipping $host"
        continue
     }
     
     set elements [array names tmp_config]
     set counter 0
     set output ""
     set removed ""
     set have_exec_spool_dir [get_local_spool_dir $host execd 0] 
     set spool_dir 0
     set expected_entries 4
     if {$have_exec_spool_dir != ""} {
        puts $CHECK_OUTPUT "host $host has spooldir in \"$have_exec_spool_dir\""
        set spool_dir 1
        incr expected_entries 1
     }

     set spool_dir_found 0
     set win_execd_params_found 0
     foreach elem $elements {
        append output "$elem is set to $tmp_config($elem)\n" 
        incr counter 1
        switch $elem {
           "mailer" { continue }
           "qlogin_daemon" { continue }
           "rlogin_daemon" { continue }
           "xterm" { continue }
           "load_sensor" {
              # on windows, we have a load sensor, on other platforms not
              if {[host_conf_get_arch $host] == "win32-x86"} {
                 incr expected_entries 1
              } else {
                 lappend removed $elem
              }
           }
           "execd_params" {
              # on windows, we need a special execd param for use of domain users
              if {[host_conf_get_arch $host] == "win32-x86"} {
                 incr expected_entries 1
                 if {$tmp_config(execd_params) == "enable_windomacc=true"} {
                    set win_execd_params_found 1
                 }
              } else {
                 lappend removed $elem
              }
           }
           "execd_spool_dir" {
              if { [string compare $have_exec_spool_dir $tmp_config(execd_spool_dir)] == 0 } {
                  set spool_dir_found 1
              }
              if { $spool_dir == 0 } {
                 lappend removed $elem
              }
           }
           default {
              lappend removed $elem
           }
        }
     }

     # execd_spool_dir has to be set correctly (depending on testsuite configuration)
     if { $spool_dir == 1 && $spool_dir_found == 0 } {
        add_proc_error "setup_execd_conf" -3 "host $host should have spool dir entry \"$have_exec_spool_dir\"\nADDING: execd_spool_dir $have_exec_spool_dir"
        if {[info exists tmp_config(execd_spool_dir)]} {
           puts $CHECK_OUTPUT "spooldir (old): $tmp_config(execd_spool_dir)"
        } else {
           puts $CHECK_OUTPUT "spooldir (old): <not set>"
        }
        set tmp_config(execd_spool_dir) $have_exec_spool_dir
        puts $CHECK_OUTPUT "spooldir (new): $tmp_config(execd_spool_dir)"
     }
     puts $CHECK_OUTPUT $output
     if { $counter != $expected_entries } {
        add_proc_error "setup_execd_conf" -1 "host $host has $counter from $expected_entries expected entries:\n$output"
     }

     # we need execd params for windows hosts
     if {[host_conf_get_arch $host] == "win32-x86" && !$win_execd_params_found} {
        set tmp_config(execd_params) "enable_windomacc=true"
     }
    
     # remove unexpected options
     foreach elem $removed {
        set tmp_config($elem) ""
     }

     # now set the new config
     set_config tmp_config $host
  }

  set_error 0 "ok"
}




#                                                             max. column:     |
#****** install_core_system/setup_mytestproject() ******
# 
#  NAME
#     setup_mytestproject -- ??? 
#
#  SYNOPSIS
#     setup_mytestproject { } 
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
proc setup_mytestproject {} {
   global ts_config
  global CHECK_ARCH env check_errno
 

  if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     set_error 0 "setup_mytestproject - not possible for sge systems"
     return
  }

  # setup project "mytestproject"
  set prj_setup(name) "mytestproject"
  set result [ add_prj prj_setup ] 

  set_error 0 "ok"  
}



#                                                             max. column:     |
#****** install_core_system/setup_mytestpe() ******
# 
#  NAME
#     setup_mytestpe -- ??? 
#
#  SYNOPSIS
#     setup_mytestpe { } 
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
proc setup_mytestpe {} {
   global ts_config

   set change(pe_name) "mytestpe"
   set change(slots) "5"
   add_pe change

   assign_queues_with_pe_object "all.q" "" "mytestpe"

   set_error 0 "ok"
}



#                                                             max. column:     |
#****** install_core_system/setup_deadlineuser() ******
# 
#  NAME
#     setup_deadlineuser -- ??? 
#
#  SYNOPSIS
#     setup_deadlineuser { } 
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
proc setup_deadlineuser {} {
   global ts_config
  global CHECK_ARCH CHECK_USER

  add_access_list $CHECK_USER deadlineusers

  set_error 0 "ok"
}


#                                                             max. column:     |
#****** install_core_system/setup_schedconf() ******
# 
#  NAME
#     setup_schedconf -- ??? 
#
#  SYNOPSIS
#     setup_schedconf { } 
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
proc setup_schedconf {} {
   global ts_config
  global CHECK_ARCH env check_errno CHECK_USER
  global CHECK_OUTPUT

  # always create global complex list if not existing
  catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-scl" } result

  puts $CHECK_OUTPUT "result: $result"

  if { [string first "Usage" $result] >= 0 } {
     puts $CHECK_OUTPUT "WARNING: ignore complex setup !!!"
  } else {
     if { [string first "global" $result] >= 0 } {
        puts $CHECK_OUTPUT "complex list global already exists"
     } else {
        if { [get_complex_version] == 0 } {
           puts $CHECK_OUTPUT "creating global complex list"
           set host_complex(complex1) "c1 DOUBLE 55.55 <= yes yes 0"
           set_complex host_complex global 1
           set host_complex(complex1) ""
           set_complex host_complex global
        }
     }

  }


  # reset_schedd_config has global error reporting
  get_schedd_config old_config
  reset_schedd_config 
  get_schedd_config new_config

  set arrays_old [ array names old_config ]
  set arrays_new [ array names new_config ]

  if { [ llength $arrays_old] == [ llength $arrays_new ] } {
    foreach param $arrays_old {
       set old  $old_config($param)
       set new  $new_config($param)

       if { [ string compare $old $new ] != 0 } {
          if { $ts_config(gridengine_version) == 53 } {
             if { [ string compare $param "sgeee_schedule_interval" ] == 0 } { continue }
          } else {
             if { [ string compare $param "reprioritize_interval" ] == 0 } { continue }
          }             
          if { [ string compare $param "weight_tickets_deadline" ] == 0 } { continue }
          if { [ string compare $param "job_load_adjustments" ] == 0 } { continue }
          if { [ string compare $param "schedule_interval" ] == 0 } { continue }
          add_proc_error "setup_schedconf" -3 "scheduler parameter $param:\ndefault setup: $old, after testsuite reset: $new" 
       }
    }
  } else {
     add_proc_error "setup_schedconf" -1 "parameter count new/old scheduler configuration error"
  }
  set_error 0 "ok"
}



#                                                             max. column:     |
#****** install_core_system/setup_default_calendars() ******
# 
#  NAME
#     setup_default_calendars -- ??? 
#
#  SYNOPSIS
#     setup_default_calendars { } 
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
proc setup_default_calendars {} {
   global ts_config
  global CHECK_ARCH env check_errno CHECK_USER


  set calendar_param(calendar_name)          "always_suspend"              ;# always in calendar suspend
  set calendar_param(year)                   "NONE"
  set calendar_param(week)                   "mon-sun=0-24=suspended"

  set result [ add_calendar calendar_param ]
  if { $result != 0 } {
     set_error -1 "setup_default_calendards: result of add_default_calendars: $result"
     return
  }

  set calendar_param(calendar_name)          "always_disabled"              ;# always in calendar suspend
  set calendar_param(year)                   "NONE"
  set calendar_param(week)                   "mon-sun=0-24=off"

  set result [add_calendar calendar_param]
  if { $result != 0 } {
     set_error -1 "setup_default_calendars: result of add_calendar: $result"
     return
  }
 
  set_error 0 "setup_default_calendars - no errors" 
}

#                                                             max. column:     |
#****** install_core_system/setup_check_user_permissions() ******
# 
#  NAME
#     setup_check_user_permissions -- ??? 
#
#  SYNOPSIS
#     setup_check_user_permissions { } 
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
proc setup_check_user_permissions {} {
   global ts_config
   global CHECK_USER CHECK_FIRST_FOREIGN_SYSTEM_USER CHECK_SECOND_FOREIGN_SYSTEM_USER
   global CHECK_OUTPUT
   global CHECK_SCRIPT_FILE_DIR CHECK_ADMIN_USER_SYSTEM
   global check_use_installed_system 

   if {$check_use_installed_system} {
      set_error 0 "ok"
      return
   }

  set time [timestamp]
  if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
     set user_list "root $CHECK_USER $CHECK_FIRST_FOREIGN_SYSTEM_USER $CHECK_SECOND_FOREIGN_SYSTEM_USER"
  } else {
     set user_list "$CHECK_USER"
  }
  foreach user $user_list {
     puts $CHECK_OUTPUT "\n----mask-check----\nuser: $user"
     set my_command ""
     foreach host $ts_config(execd_nodes) {
        set execd_spooldir [get_execd_spool_dir $host]
        puts $CHECK_OUTPUT "checking execd spool directory on $host (user=$user): \"$execd_spooldir\""
        set output [ start_remote_prog "$host" "$user" "cd" "$execd_spooldir" ]
        if { $prg_exit_state != 0 } {
           set_error -1 "user $user has no read//exec permission to \"$execd_spooldir\" on host $host: $output"
        }
        set output [ start_remote_prog "$host" "$user" "cd" "$ts_config(testsuite_root_dir)/$CHECK_SCRIPT_FILE_DIR" ]
        if { $prg_exit_state != 0 } {
           set_error -1 "user $user has no read//exec permission to \"$ts_config(testsuite_root_dir)/$CHECK_SCRIPT_FILE_DIR\" on host $host: $output"
        }
     }
  }

  set master_spooldir [get_qmaster_spool_dir]
  puts $CHECK_OUTPUT "master spool directory on host $ts_config(master_host): \"$master_spooldir\""
  foreach user $user_list {
      puts $CHECK_OUTPUT "checking master spool directory for user: \"$user\""
      set output [ start_remote_prog "$ts_config(master_host)" "$user" "cd" "$master_spooldir"  ]
      if { $prg_exit_state != 0 } {
         puts $CHECK_OUTPUT "--> E R R O R - user $user has no read//exec permission to $master_spooldir on host $ts_config(master_host)"
         set_error -1 "user $user has no read//exec permission to $master_spooldir on host $ts_config(master_host)"
      }
  }
  puts "runtime: [ expr ( [timestamp] - $time ) ]"
  get_version_info
  set_error 0 "ok"
}


proc setup_check_messages_files {} {
   global ts_config
   global CHECK_OUTPUT
   global CHECK_USER
   global check_use_installed_system 

   if {$check_use_installed_system} {
      set_error 0 "ok"
      return
   }

   puts $CHECK_OUTPUT "scheduler ..."
   set messages [get_schedd_messages_file]
   get_file_content $ts_config(master_host) $CHECK_USER $messages 
   if { $file_array(0) < 1 } {
      add_proc_error "setup_check_messages_files" -1 "no scheduler messages file:\n$messages"
   }
   for {set i 1 } { $i <= $file_array(0) } { incr i 1 } {
      puts $CHECK_OUTPUT $file_array($i)
   }

   puts $CHECK_OUTPUT "qmaster ..."
   set messages [get_qmaster_messages_file]
   get_file_content $ts_config(master_host) $CHECK_USER $messages 
   if { $file_array(0) < 1 } {
      add_proc_error "setup_check_messages_files" -1 "no qmaster messages file:\n$messages"
   }
   for {set i 1 } { $i <= $file_array(0) } { incr i 1 } {
      puts $CHECK_OUTPUT $file_array($i)
   }

   foreach execd $ts_config(execd_nodes) {
      puts $CHECK_OUTPUT "execd $execd ..."
      set messages [ get_execd_messages_file $execd ]
      get_file_content $execd $CHECK_USER $messages 
      if { $file_array(0) < 1 } {
         add_proc_error "setup_check_messages_files" -1 "no execd(host=$execd) messages file:\n$messages"
      }
      for {set i 1 } { $i <= $file_array(0) } { incr i 1 } {
         puts $CHECK_OUTPUT $file_array($i)
      }
   }
   set_error 0 "ok"
}

#                                                             max. column:     |
#****** install_core_system/setup_inhouse_cluster() ******
# 
#  NAME
#     setup_inhouse_cluster -- ??? 
#
#  SYNOPSIS
#     setup_inhouse_cluster { } 
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
proc setup_inhouse_cluster {} {
   global ts_config
  global CHECK_ARCH env check_errno CHECK_USER
  global CHECK_OUTPUT

  # reset_schedd_config has global error reporting
  if {[lsearch -exact [info procs] "inhouse_cluster_post_install"] != -1} {
    puts $CHECK_OUTPUT "executing postinstall procedure for inhouse cluster"
    inhouse_cluster_post_install
  }

  set_error 0 "ok"
}

#****** init_cluster/setup_win_users() *****************************************
#  NAME
#     setup_win_users() -- special setup for windows users
#
#  SYNOPSIS
#     setup_win_users { } 
#
#  FUNCTION
#     If we have windows hosts in the cluster, this procedure does special setup
#     required for windows users.
#     The passwords of the following users will be registered through the
#     sgepasswd utilbin binary:
#        - CHECK_USER
#        - Administrator
#        - CHECK_FIRST_FOREIGN_SYSTEM_USER
#        - CHECK_SECOND_FOREIGN_SYSTEM_USER
#
#  SEE ALSO
#     init_cluster/setup_win_user()
#*******************************************************************************
proc setup_win_users {} {
   global ts_config CHECK_OUTPUT
   global CHECK_USER CHECK_FIRST_FOREIGN_SYSTEM_USER CHECK_SECOND_FOREIGN_SYSTEM_USER

   if {[host_conf_have_windows]} {
      set win_users "$CHECK_USER Administrator $CHECK_FIRST_FOREIGN_SYSTEM_USER $CHECK_SECOND_FOREIGN_SYSTEM_USER"
      foreach user $win_users {
         setup_win_user_passwd $user
      }
   }

   set_error 0 "ok"
}

#****** init_cluster/setup_win_user_passwd() ***********************************
#  NAME
#     setup_win_user_passwd() -- register the passwd of a windows user
#
#  SYNOPSIS
#     setup_win_user_passwd { user } 
#
#  FUNCTION
#     Registeres the passwd of a given windows user by calling the sgepasswd
#     utilbin binary and answering the password questions.
#  
#     Requires that passwords have been interactively entered through the
#     set_root_passwd procedure.
#
#  INPUTS
#     user - user whose passwd shall be registered
#
#  SEE ALSO
#     init_cluster/setup_win_users()
#     check/set_root_passwd()
#*******************************************************************************
proc setup_win_user_passwd {user} {
   global ts_config CHECK_OUTPUT
   global CHECK_USER CHECK_DEBUG_LEVEL

   puts -nonewline $CHECK_OUTPUT "setting sgepasswd of user $user ..."
   
   set id [open_remote_spawn_process $ts_config(master_host) $CHECK_USER "sgepasswd" $user]
   set sp_id [lindex $id 1]

   # in debug mode we want to see all the shell output
   log_user 0
   if {$CHECK_DEBUG_LEVEL != 0} {
      log_user 1
      puts $CHECK_OUTPUT ""
   }

   # wait for and answer passwd questions
   set timeout 60
   expect {
      -i $sp_id full_buffer {
         add_proc_error "setup_win_user_passwd" -1 "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
         close_spawn_process $id
         return
      }   

      -i $sp_id eof { 
         add_proc_error "setup_win_user_passwd" "-1" "unexpected eof"
         close_spawn_process $id
         return
      }

      -i $sp_id timeout { 
         add_proc_error "setup_win_user_passwd" "-1" "timeout while waiting for password question"
         close_spawn_process $id;
         return
      }

      -i $sp_id "password:" {
         send -i $sp_id "[get_passwd $user]\n"
         exp_continue
      }
      -i $sp_id "Password changed" {
         puts -nonewline $CHECK_OUTPUT " + "
         exp_continue
      }
      -i $sp_id "_exit_status_:" {
         puts $CHECK_OUTPUT "done"
      }
   }

   # cleanup
   close_spawn_process $id
}

proc cleanup_tmpdirs {} {
   global ts_config CHECK_OUTPUT
   global CHECK_ADMIN_USER_SYSTEM CHECK_USER

   set tmpdir "/tmp/testsuite_$ts_config(commd_port)"

   # in an admin user system (no root password available)
   # we have to cleanup the tmpdir as CHECK_USER
   # this shouldn't matter, as there shouldn't be subdirectories
   # created by another user than CHECK_USER
   if {$CHECK_ADMIN_USER_SYSTEM} {
      set clean_user $CHECK_USER
   } else {
      set clean_user "root"
   }

   foreach node $ts_config(execd_nodes) {
      puts $CHECK_OUTPUT "cleaning tmpdir ($tmpdir) on node $node"
      start_remote_prog $node $clean_user "rm" "-rf $tmpdir"
   }
}
