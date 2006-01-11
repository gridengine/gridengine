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

proc validate_checkpointobj { change_array } {
   global CHECK_OUTPUT

   upvar $change_array chgar

  if { [info exists chgar(queue_list)] } {
     puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for ckpt objects"
     add_proc_error "validate_checkpointobj" -3 "this Grid Engine version doesn' t support a queue_list for ckpt objects,\nuse assign_queues_with_ckpt_object() after adding checkpoint\nobjects and don't use queue_list parameter."
     unset chgar(queue_list)
  }
}


#****** sge_checkpoint.60/mod_checkpointobj() *************************************
#  NAME
#     mod_checkpointobj() -- Modify checkpoint object configuration
#
#  SYNOPSIS
#     mod_checkpointobj { ckpt_obj change_array {fast_add 1}}
#
#  FUNCTION
#     Modify a checkpoint configuration corresponding to the content of the
#     change_array.
#
#  INPUTS
#     ckpt_obj     - name of the checkpoint object to configure
#     change_array - name of array variable that will be set by
#                    mod_checkpointobj()
#     {fast_add 1} - 0: modify the attribute using qconf -mckpt,
#                  - 1: modify the attribute using qconf -Mckpt, faster
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  SEE ALSO
#     sge_checkpoint/get_checkpointobj()
#*******************************************************************************
proc mod_checkpointobj { ckpt_obj change_array {fast_add 1}} {
 global ts_config
   global CHECK_ARCH open_spawn_buffer
   global CHECK_USER CHECK_OUTPUT CHECK_HOST

   upvar $change_array chgar

   validate_checkpointobj chgar

   # add queue from file?
   if { $fast_add } {
      set tmpfile [dump_array_to_tmpfile default_array]
      set result [start_sge_bin "qconf" "-Mckpt ${tmpfile}"]

   } else {

      set vi_commands [build_vi_command chgar]
      set args "-mckpt $ckpt_obj"

      set ALREADY_EXISTS [ translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" $ckpt_obj]
      set MODIFIED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $ckpt_obj "checkpoint interface" ]

       set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" $args $vi_commands $MODIFIED $ALREADY_EXISTS ]
    

      if { $result == -1 } { add_proc_error "mod_checkpointobj" -1 "timeout error" }
      if { $result == -2 } { add_proc_error "mod_checkpointobj" -1 "already exists" }
      if { $result == -3 } { add_proc_error "mod_checkpointobj" -1 "queue reference does not exist" }
      if { $result != 0  } { add_proc_error "mod_checkpointobj" -1 "could not modify checkpoint object" }
   }

   return $result
}

