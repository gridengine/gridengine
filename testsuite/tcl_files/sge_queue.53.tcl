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

proc get_queue_instance {queue host} {
   return "${queue}_${host}"
}

#****** sge_procedures.53/queue/vdep_set_queue_defaults() **********************
#  NAME
#     vdep_set_queue_defaults() -- create version dependent queue settings
#
#  SYNOPSIS
#     vdep_set_queue_defaults { change_array } 
#
#  FUNCTION
#     Fills the array change_array with queue attributes specific for SGE 5.3
#
#  INPUTS
#     change_array - the resulting array
#
#  SEE ALSO
#     sge_procedures/queue/set_queue_defaults()
#*******************************************************************************
proc vdep_set_queue_defaults { change_array } {
   global ts_config
   upvar $change_array chgar

   set chgar(hostname)             "hostname"
   set chgar(qtype)                "BATCH INTERACTIVE CHECKPOINTING PARALLEL"
   set chgar(complex_list)         "NONE"
   if { $ts_config(product_type) == "sgeee" } {
      set chgar(fshare)             "0"
      set chgar(oticket)            "0"
   }
}

#****** sge_procedures.53/queue/validate_queue_type() ********************************
#  NAME
#     validate_queue_type() -- validate the settings for queue_type
#
#  SYNOPSIS
#     validate_queue_type { change_array } 
#
#  FUNCTION
#     No action for SGE 5.3.
#
#  INPUTS
#     change_array - array containing queue definitions
#
#*******************************************************************************
proc validate_queue_type { change_array } {
   # nothing to be done for SGE 5.3
}


#****** sge_procedures.53/queue/add_queue() ******************************************
#  NAME
#     add_queue() -- add a SGE 5.3 queue
#
#  SYNOPSIS
#     add_queue { qname hostlist change_array {fast_add 0} } 
#
#  FUNCTION
#     Adds one or multiple queues to a SGE 5.3 system.
#     Queue names are created as $qname_$hostname.
#
#  INPUTS
#     qname        - name for the (cluster) queue
#     hostlist     - list of hostnames, or "@allhosts" to create a queue on each host.
#     change_array - array containing attributes that differ from defaults
#     {fast_add 0} - 0: add the queue using qconf -aq,
#                    1: add the queue using qconf -Aq, much faster!
#
#  RESULT
#
#*******************************************************************************
proc add_queue { qname hostlist change_array {fast_add 0} } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER
   global open_spawn_buffer

   upvar $change_array chgar

   # queue_type is version dependent
   validate_queue_type chgar

   # non cluster queue: set queue and hostnames
   if { $hostlist == "@allhosts" || $hostlist == "" } {
      set hostlist $ts_config(execd_hosts)
   }

   # localize messages
   set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]

   foreach host $hostlist {
      puts $CHECK_OUTPUT "creating queue \"$qname\" for host \"$host\""

      set chgar(qname)     [get_queue_instance ${qname} ${host}]
      set chgar(hostname)  "$host"

      # localize messages containing the queue name
      set ALREADY_EXISTS [ translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] $QUEUE $chgar(qname)]
      set ADDED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $chgar(qname) $QUEUE ]

      # add queue from file?
      if { $fast_add } {
         set_queue_defaults default_array
         update_change_array default_array chgar

         set tmpfile [dump_array_to_tmpfile default_array]

         set result ""
         set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -Aq ${tmpfile}" } result ]
         puts $CHECK_OUTPUT $result

         if { [string match "*$ADDED" $result ] == 0 } {
            add_proc_error "add_queue" "-1" "qconf error or binary not found"
            break
         } 
      } else {
         # add by handling vi
         set vi_commands [build_vi_command chgar]

         set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aq" $vi_commands $ADDED $ALREADY_EXISTS ]  
         if { $result != 0 } {
            add_proc_error "add_queue" -1 "could not add queue $chgar(qname) (error: $result)"
            break
         }
      }
   }

   return $result
}

# set_queue_work - no public interface
proc set_queue_work { qname change_array } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH CHECK_USER

   upvar $change_array chgar

   puts $CHECK_OUTPUT "modifying queue \"$qname\""

   set vi_commands [build_vi_command chgar]

   set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
   set NOT_A_QUEUENAME [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QUEUE_XISNOTAQUEUENAME_S] $qname ]
   set MODIFIED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $qname $QUEUE ]
   set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mq ${qname}" $vi_commands $MODIFIED $NOT_A_QUEUENAME]
   if { $result == -2 } {
      add_proc_error "set_queue" -1 "$qname is not a queue"
   }
   if { $result != 0  } {
      add_proc_error "set_queue" -1 "error modify queue $qname, $result"
   } 

   return $result
}

#****** sge_procedures.53/queue/set_queue() ******************************************
#  NAME
#     set_queue() -- set queue attributes
#
#  SYNOPSIS
#     set_queue { qname hostlist change_array } 
#
#  FUNCTION
#     Sets the attributes given in change_array in the queues specified by
#     qname and hostlist.
#     Queuenames are built as $qname_$hostname.
#
#  INPUTS
#     qname        - name of the (cluster) queue
#     hostlist     - list of hosts. If "@allhosts" is given, the attributes are changed
#                    for all hosts. If an empty list is given, the queuename is only
#                    built from the qname parameter.
#     change_array - array containing the changed attributes.
#
#  RESULT
#
#*******************************************************************************
proc set_queue { qname hostlist change_array } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER
   global open_spawn_buffer

   upvar $change_array chgar

   # queue_type is version dependent
   validate_queue_type chgar

   # non cluster queue: set queue and hostnames
   if { $hostlist == "@allhosts" || $hostlist == "" } {
      set hostlist $ts_config(execd_hosts)
   }

   foreach host $hostlist {
      set cqname [get_queue_instance ${qname} ${host}]
      set result [set_queue_work $cqname chgar]
   }

   return $result
}

proc del_queue { q_name hostlist {ignore_hostlist 0} {del_cqueue 0}} {
  global ts_config
  global CHECK_ARCH open_spawn_buffer CHECK_CORE_MASTER CHECK_CORE_EXECD CHECK_USER CHECK_OUTPUT CHECK_HOST

   # we just get one queue name (queue instance)
   set queue_list {}
   if { $ignore_hostlist } {
      lappend queue_list $q_name
   } else {
      # we get a cluster queue name and a hostlist
      if { $hostlist == "" } {
         hostlist = $CHECK_CORE_EXECD
      }
      foreach host $hostlist {
         lappend queue_list [get_queue_instance $q_name $host]
      }
   }

   foreach queue $queue_list {
      set result ""
      set catch_return [ catch {  
         eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -dq ${queue}" 
      } result ]

      set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
      set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $queue $QUEUE ]

      if { [string match "*$REMOVED" $result ] == 0 } {
         add_proc_error "del_queue" "-1" "could not delete queue $queue: (error: $result)"
         return -1
      } 
  }

  return 0
}

proc get_queue_list {} {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   set NO_QUEUE_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "queue"]

   # try to get queue list
   if { [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sql" } result] != 0 } {
      # if command fails: output error
      add_proc_error "get_queue_list" -1 "error reading queue list: $result"
      set result {}
   } else {
      # command succeeded: queue list can be empty
      if { [string first $NO_QUEUE_DEFINED $result] >= 0 } {
         puts $CHECK_OUTPUT $result
         set result {}
      }
   }

   return $result
}

# queue for -q request or as subordinate queue
# is the 5.3 queue
proc get_requestable_queue { queue host } {
   return [get_queue_instance $queue $host]
}
