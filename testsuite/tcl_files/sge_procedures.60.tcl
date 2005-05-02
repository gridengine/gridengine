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

proc unassign_queues_with_pe_object { pe_obj } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   puts $CHECK_OUTPUT "searching for references in cluster queues ..."
   set queue_list [get_queue_list]
   foreach elem $queue_list {
      puts $CHECK_OUTPUT "queue: $elem"
      if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dattr" "queue" "pe_list" "$pe_obj" "$elem" } result] != 0 } {
         # if command fails: output error
         add_proc_error "unassign_queues_with_pe_object" -1 "error reading queue list: $result"
      }
   }
   puts $CHECK_OUTPUT "searching for references in queue instances ..."
   set queue_list [get_qinstance_list "-pe $pe_obj"]
   foreach elem $queue_list {
      puts $CHECK_OUTPUT "queue: $elem"
      if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dattr" "queue" "pe_list" "$pe_obj" "$elem" } result] != 0 } {
         # if command fails: output error
         add_proc_error "unassign_queues_with_pe_object" -1 "error changing pe_list: $result"
      }
   }
}

#****** sge_procedures.60/copy_certificates() **********************************
#  NAME
#     copy_certificates() -- copy csp (ssl) certificates to the specified host
#
#  SYNOPSIS
#     copy_certificates { host } 
#
#  FUNCTION
#     copy csp (ssl) certificates to the specified host
#
#  INPUTS
#     host - host where the certificates has to be copied. (Master installation
#            must be called before)
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc copy_certificates { host } {
   global CHECK_OUTPUT ts_config CHECK_ADMIN_USER_SYSTEM CHECK_USER

   set remote_arch [resolve_arch $host]
   
   puts $CHECK_OUTPUT "installing CA keys"
   puts $CHECK_OUTPUT "=================="
   puts $CHECK_OUTPUT "host:         $host"
   puts $CHECK_OUTPUT "architecture: $remote_arch"
   puts $CHECK_OUTPUT "port:         $ts_config(commd_port)"
   puts $CHECK_OUTPUT "source:       \"/var/sgeCA/port${ts_config(commd_port)}/\" on host $ts_config(master_host)"
   puts $CHECK_OUTPUT "target:       \"/var/sgeCA/port${ts_config(commd_port)}/\" on host $host"
  
   if { $CHECK_ADMIN_USER_SYSTEM == 0 } {
      puts $CHECK_OUTPUT "we have root access, fine !"
      set CA_ROOT_DIR "/var/sgeCA"
      set TAR_FILE "${CA_ROOT_DIR}/port${ts_config(commd_port)}.tar"

      puts $CHECK_OUTPUT "removing existing tar file \"$TAR_FILE\" ..."
      set result [ start_remote_prog "$ts_config(master_host)" "root" "rm" "$TAR_FILE" ]
      puts $CHECK_OUTPUT $result

      puts $CHECK_OUTPUT "taring Certificate Authority (CA) directory into \"$TAR_FILE\""
      set tar_bin [get_binary_path $ts_config(master_host) "tar"]
      set remote_command_param "$CA_ROOT_DIR; ${tar_bin} -cvf $TAR_FILE ./port${ts_config(commd_port)}/*"
      set result [ start_remote_prog "$ts_config(master_host)" "root" "cd" "$remote_command_param" ]
      puts $CHECK_OUTPUT $result

      if { $prg_exit_state != 0 } {
         add_proc_error "copy_certificates" -2 "could not tar Certificate Authority (CA) directory into \"$TAR_FILE\""
      } else {
         puts $CHECK_OUTPUT "copy tar file \"$TAR_FILE\"\nto \"$ts_config(results_dir)/port${ts_config(commd_port)}.tar\" ..."
         set result [ start_remote_prog "$ts_config(master_host)" "$CHECK_USER" "cp" "$TAR_FILE $ts_config(results_dir)/port${ts_config(commd_port)}.tar" prg_exit_state 300 ]
         puts $CHECK_OUTPUT $result
                    
         # tar file will be on nfs - wait for it to be visible
         wait_for_remote_file $host $CHECK_USER "$ts_config(results_dir)/port${ts_config(commd_port)}.tar"
                    
         puts $CHECK_OUTPUT "copy tar file \"$ts_config(results_dir)/port${ts_config(commd_port)}.tar\"\nto \"$TAR_FILE\" on host $host ..."
         set result [ start_remote_prog "$host" "root" "cp" "$ts_config(results_dir)/port${ts_config(commd_port)}.tar $TAR_FILE" prg_exit_state 300 ]
         puts $CHECK_OUTPUT $result

         set tar_bin [get_binary_path $host "tar"]

         puts $CHECK_OUTPUT "untaring Certificate Authority (CA) directory in \"$CA_ROOT_DIR\""
         start_remote_prog "$host" "root" "cd" "$CA_ROOT_DIR" 
         if { $prg_exit_state != 0 } { 
            set result [ start_remote_prog "$host" "root" "mkdir" "-p $CA_ROOT_DIR" ]
         }   

         set result [ start_remote_prog "$host" "root" "cd" "$CA_ROOT_DIR; ${tar_bin} -xvf $TAR_FILE" prg_exit_state 300 ]
         puts $CHECK_OUTPUT $result
         if { $prg_exit_state != 0 } {
            add_proc_error "copy_certificates" -2 "could not untar \"$TAR_FILE\" on host $host;\ntar-bin:$tar_bin"
         } 

         puts $CHECK_OUTPUT "removing tar file \"$TAR_FILE\" on host $host ..."
         set result [ start_remote_prog "$host" "root" "rm" "$TAR_FILE" ]
         puts $CHECK_OUTPUT $result

         puts $CHECK_OUTPUT "removing tar file \"$ts_config(results_dir)/port${ts_config(commd_port)}.tar\" ..."
         set result [ start_remote_prog "$ts_config(master_host)" "$CHECK_USER" "rm" "$ts_config(results_dir)/port${ts_config(commd_port)}.tar" ]
         puts $CHECK_OUTPUT $result
      }
                
      puts $CHECK_OUTPUT "removing tar file \"$TAR_FILE\" ..."
      set result [ start_remote_prog "$ts_config(master_host)" "root" "rm" "$TAR_FILE" ]
      puts $CHECK_OUTPUT $result

      # check for syncron clock times
      set my_timeout [timestamp]
      incr my_timeout 600
      while { 1 } {
         set result [start_remote_prog $host $CHECK_USER "$ts_config(product_root)/bin/$remote_arch/qstat" "-f"]
         puts $CHECK_OUTPUT $result
         if { $prg_exit_state == 0 } {
            puts $CHECK_OUTPUT "qstat -f works, fine!"
            break
         }
         puts $CHECK_OUTPUT "waiting for qstat -f to work ..."
         puts $CHECK_OUTPUT "please check hosts for synchron clock times"
         sleep 10
         if { [timestamp] > $my_timeout } {
            add_proc_error "copy_certificates" -2 "$host: timeout while waiting for qstat to work (please check hosts for synchron clock times)"
         }
      } 
   } else {
      puts $CHECK_OUTPUT "can not copy this files as user $CHECK_USER"
      puts $CHECK_OUTPUT "installation error"
      add_proc_error "copy_certificates" -2 "$host: can't copy certificate files as user $CHECK_USER"
   }
}

proc unassign_queues_with_ckpt_object { ckpt_obj } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   puts $CHECK_OUTPUT "searching for references in cluster queues ..."
   set queue_list [get_queue_list]
   foreach elem $queue_list {
      puts $CHECK_OUTPUT "queue: $elem"
      if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dattr" "queue" "ckpt_list" "$ckpt_obj" "$elem" } result] != 0 } {
         # if command fails: output error
         add_proc_error "unassign_queues_with_ckpt_object" -1 "error reading queue list: $result"
      }
   }
   puts $CHECK_OUTPUT "searching for references in queue instances ..."
   set queue_list [get_qinstance_list]
   foreach elem $queue_list {
      puts $CHECK_OUTPUT "queue: $elem"
      if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dattr" "queue" "ckpt_list" "$ckpt_obj" "$elem" } result] != 0 } {
         # if command fails: output error
         add_proc_error "unassign_queues_with_ckpt_object" -1 "error changing ckpt_list: $result"
      }
   }
}


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

proc set_complex { change_array } {
  global ts_config CHECK_USER
  global env CHECK_ARCH CHECK_OUTPUT open_spawn_buffer
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
        lappend vi_commands "A\n$elem  $newVal"
        lappend vi_commands [format "%c" 27]
     }
  }

#  foreach vi_com $vi_commands {
#     puts $CHECK_OUTPUT "\"$vi_com\""
#  }

  set EDIT_FAILED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PARSE_EDITFAILED]]
  set MODIFIED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ADDED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set result [ handle_vi_edit "echo" "\"\"\nSGE_ENABLE_MSG_ID=1\nexport SGE_ENABLE_MSG_ID\n$ts_config(product_root)/bin/$CHECK_ARCH/qconf -mc" $vi_commands $MODIFIED $REMOVED $ADDED ]
  if { $result != 0 && $result != -2 && $result != -3 && $result != -4 } {
     add_proc_error "set_complex" -1 "could not modify complex: ($result)"
  }
  return $result
}


proc assign_queues_with_ckpt_object { qname hostlist ckpt_obj } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   set queue_list {}
   # if we have no hostlist: change cluster queue
   if {[llength $hostlist] == 0} {
      set queue_list $qname
   } else {
      foreach host $hostlist {
         lappend queue_list "${qname}@${host}"
      }
   }

   foreach queue $queue_list {
      puts $CHECK_OUTPUT "queue: $queue"
      if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aattr" "queue" "ckpt_list" "$ckpt_obj" "$queue" } result] != 0 } {
         # if command fails: output error
         add_proc_error "assign_queues_with_ckpt_object" -1 "error changing ckpt_list: $result"
      }
   }
}

proc assign_queues_with_pe_object { qname hostlist pe_obj } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   set queue_list {}
   # if we have no hostlist: change cluster queue
   if {[llength $hostlist] == 0} {
      set queue_list $qname
   } else {
      foreach host $hostlist {
         lappend queue_list "${qname}@${host}"
      }
   }

   foreach queue $queue_list {
      puts $CHECK_OUTPUT "queue: $queue"
      if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aattr" "queue" "pe_list" "$pe_obj" "$queue" } result] != 0 } {
         # if command fails: output error
         add_proc_error "assign_queues_with_pe_object" -1 "error changing pe_list: $result"
      }
   }
}

proc validate_checkpointobj { change_array } {
   global CHECK_OUTPUT

   upvar $change_array chgar

  if { [info exists chgar(queue_list)] } { 
     puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for ckpt objects"
     add_proc_error "validate_checkpointobj" -3 "this Grid Engine version doesn't support a queue_list for ckpt objects,\nuse assign_queues_with_ckpt_object() after adding checkpoint\nobjects and don't use queue_list parameter."
     unset chgar(queue_list)
  }
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
  global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER


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

   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/sgemaster" "-shadowd start"]
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
proc startup_execd { hostname } {
   global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER

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
   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/sgeexecd" "start"]

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

