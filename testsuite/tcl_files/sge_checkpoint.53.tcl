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

proc unassign_queues_with_ckpt_object { ckpt_obj } {
   # nothing to be done for SGE 5.3
}
proc validate_checkpointobj { change_array } {
   # nothing to be done for SGE 5.3
}

proc assign_queues_with_ckpt_object { qname hostlist ckpt_obj } {
   global ts_config
   global CHECK_OUTPUT

   if { $hostlist == "" } {
      set hostlist $ts_config(execd_nodes)
   }

   # set queue_list in checkpoint object
   set q_list ""
   foreach host $hostlist {
      set queue [get_queue_instance $qname $host]
      if { [string length $q_list] > 0} {
         set q_list "$q_list,$queue"
      } else {
         set q_list "$queue"
      }
   }

   # workaround for very long queue lists: use all parameter
   if {[string length $q_list] > 256} {
      set q_list "all"
   }

   get_checkpointobj $ckpt_obj curr_ckpt
   if { $q_list == "all" || $curr_ckpt(queue_list) == "all" || $curr_ckpt(queue_list) == "NONE" } {
      set my_change(queue_list) $q_list
   } else {
      set my_change(queue_list) "$curr_ckpt(queue_list) $q_list"
   }
   set my_change(ckpt_name) $ckpt_obj
   mod_checkpointobj my_change
}

#****** sge_checkpoint.53/mod_checkpointobj() *************************************
#  NAME
#     mod_checkpointobj() -- Modify checkpoint object configuration
#
#  SYNOPSIS
#     mod_checkpointobj { change_array {fast_add 1}  {on_host ""} {as_user ""}}
#
#  FUNCTION
#     Modify a checkpoint configuration corresponding to the content of the
#     change_array.
#
#  INPUTS
#     change_array - name of array variable that will be set by
#                    mod_checkpointobj()
#     {fast_add 1} - 0: modify the attribute using qconf -mckpt,
#                  - 1: modify the attribute using qconf -Mckpt, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  SEE ALSO
#     sge_checkpoint/get_checkpointobj()
#*******************************************************************************

proc mod_checkpointobj { change_array {fast_add 1} {on_host ""} {as_user ""}} {
   global ts_config
   global CHECK_ARCH open_spawn_buffer
   global CHECK_USER CHECK_OUTPUT

   upvar $change_array chgar

   validate_checkpointobj chgar

   set ckpt_obj $chgar(ckpt_name)

   # add queue from file?
   if { $fast_add } {
      set tmpfile [dump_array_to_tmpfile chgar]
      set result [start_sge_bin "qconf" "-Mckpt $tmpfile" $on_host $as_user]

      set ret $prg_exit_state 

   } else {
      set vi_commands [build_vi_command chgar]
      set args "-mckpt $ckpt_obj"

      set ALREADY_EXISTS [ translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" $ckpt_obj]
      set MODIFIED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $ckpt_obj "checkpoint interface" ]

      set REFERENCED_IN_QUEUE_LIST_OF_CHECKPOINT [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNQUEUE_SSSS] "*" "*" "*" "*"]

      set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" $args $vi_commands $MODIFIED $ALREADY_EXISTS $REFERENCED_IN_QUEUE_LIST_OF_CHECKPOINT ]

      if { $result == -1 } { add_proc_error "mod_checkpointobj" -1 "timeout error" }
      if { $result == -2 } { add_proc_error "mod_checkpointobj" -1 "already exists" }
      if { $result == -3 } { add_proc_error "mod_checkpointobj" -1 "queue reference does not exist"
 }
      if { $result != 0  } { add_proc_error "mod_checkpointobj" -1 "could not modify checkpoint object" }

      set ret $result
   }
   return $ret
}


