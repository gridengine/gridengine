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
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER


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

   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/sgemaster" "start"]
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
   global CHECK_HOST CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER
   global CHECK_CORE_MASTER

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

