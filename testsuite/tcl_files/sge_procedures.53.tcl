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

proc validate_queue_type { change_array } {
   # nothing to be done for SGE 5.3
}


proc vdep_set_queue_defaults { change_array } {
   upvar $change_array chgar

   set chgar(hostname)             "hostname"
   set chgar(qtype)                "BATCH INTERACTIVE CHECKPOINTING PARALLEL"
   set chgar(complex_list)         "NONE"
}

proc add_queue { qname hostlist change_array {fast_add 0} } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER
   global open_spawn_buffer

   upvar $change_array chgar

   # queue_type is version dependent
   validate_queue_type chgar

   # non cluster queue: set queue and hostnames
   if { $hostlist == "@all" } {
      set hostlist $ts_config(execd_hosts)
   }

   foreach host $hostlist {
      puts $CHECK_OUTPUT "creating queue \"$qname\" for host \"$host\""

      set chgar(qname)     "${qname}_${host}"
      set chgar(hostname)  "$host"

      # add queue from file?
      if { $fast_add } {
         set_queue_defaults default_array
         vdep_set_queue_defaults default_array

         update_change_array default_array chgar

         set tmpfile [dump_array_to_tmpfile default_array]

         set result ""
         set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -Aq ${tmpfile}" } result ]
         puts $CHECK_OUTPUT $result
         set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
         set ADDED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $default_array(qname) $QUEUE ]

         if { [string match "*$ADDED" $result ] == 0 } {
            add_proc_error "add_queue" "-1" "qconf error or binary not found"
            break
         } 
      } else {
         # add by handling vi
         set vi_commands [build_vi_command chgar]

         set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
         set ALREADY_EXISTS [ translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] $QUEUE $chgar(qname)]
         set ADDED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $chgar(qname) $QUEUE ]

         set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aq" $vi_commands $ADDED $ALREADY_EXISTS ]  
         if { $result != 0 } {
            add_proc_error "add_queue" -1 "could not add queue [set chgar(qname)] (error: $result)"
            break
         }
      }
   }

   return $result
}


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

proc set_queue { qname hostlist change_array } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER
   global open_spawn_buffer

   upvar $change_array chgar

   # queue_type is version dependent
   validate_queue_type chgar

   # non cluster queue: set queue and hostnames
   if { $hostlist == "@all" } {
      set hostlist $ts_config(execd_hosts)
   }

   if { [llength $hostlist] == 0 } {
      set result [set_queue_work $qname chgar]
   } else {
      foreach host $hostlist {
         set cqname "${qname}_${host}"
         set result [set_queue_work $cqname chgar]
      }
   }

   return $result
}

proc unassign_queues_with_pe_object { pe_obj } {
   # nothing to be done for SGE 5.3
}

proc unassign_queues_with_ckpt_object { ckpt_obj } {
   # nothing to be done for SGE 5.3
}

proc assign_queues_with_ckpt_object { qname hostlist ckpt_obj } {
   global ts_config
   global CHECK_OUTPUT

   if { $hostlist == "@all" } {
      set hostlist $ts_config(execd_hosts)
   }

   # set queue_list in checkpoint object
   set q_list ""
   foreach host $hostlist {
      set queue "${qname}_${host}"
      if { [string length $q_list] > 0} {
         set q_list "$q_list,$queue"
      } else {
         set q_list "$queue"
      }
   }

   set my_change(queue_list) $q_list
   set_checkpointobj $ckpt_obj my_change
}

proc assign_queues_with_pe_object { qname hostlist pe_obj } {
   global ts_config
   global CHECK_OUTPUT

   if { $hostlist == "@all" } {
      set hostlist $ts_config(execd_hosts)
   }

   # set queue_list in checkpoint object
   set q_list ""
   foreach host $hostlist {
      set queue "${qname}_${host}"
      if { [string length $q_list] > 0} {
         set q_list "$q_list,$queue"
      } else {
         set q_list "$queue"
      }
   }

   set my_change(queue_list) $q_list
   set_pe $pe_obj my_change
}

