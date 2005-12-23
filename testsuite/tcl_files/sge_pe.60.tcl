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


