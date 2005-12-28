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

proc unassign_queues_with_pe_object { pe_obj } {
   # nothing to be done for SGE 5.3
}

proc assign_queues_with_pe_object { qname hostlist pe_obj } {
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

   get_pe $pe_obj curr_pe
   if { $q_list == "all" || $curr_pe(queue_list) == "all" || $curr_pe(queue_list) == "NONE" } {
      set my_change(queue_list) "$q_list"
   } else {
      set my_change(queue_list) "$curr_pe(queue_list) $q_list"
   }

   set_pe $pe_obj my_change
}


