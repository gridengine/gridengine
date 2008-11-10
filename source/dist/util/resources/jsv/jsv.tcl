#!/usr/bin/tclsh
#
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
#  Copyright: 2008 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

########################################################################### 
#
# example for a job verification script 
#
# Be careful:  Job verification scripts are started with sgeadmin 
#              permissions if they are executed within the master process
#

set sge_root $env(SGE_ROOT)
set sge_arch [exec $sge_root/util/arch]

source "$sge_root/util/resources/jsv/jsv_include.tcl"

proc jsv_on_start {} {
   jsv_send_env
}

proc jsv_on_verify {} {
   set do_correct 0
   set do_wait 0

   if {[string compare [jsv_get_param "b"] "y"] == 0} {
      jsv_reject "Binary job is rejected."
      return
   }

   if {[string compare [jsv_get_param "pe_name"] ""] != 0} {
      set slots [jsv_get_param "pe_slots"]
      set i [expr $slots % 16]

      if {$i > 0} {
         jsv_reject "Parallel job does not request a multiple of 16 slots"
      }
   }
  
   if {[jsv_is_param "l_hard"] == 1} {
      set context [jsv_get_param "CONTEXT"]
      set has_h_vmem [jsv_sub_is_param "l_hard" "h_vmem"]
      set has_h_data [jsv_sub_is_param "l_hard" "h_data"]

      if {$has_h_vmem == 1} {
         jsv_sub_del_param "l_hard" "h_vmem"
         set do_wait 1
         if {[string compare "client" $context] == 0} {
            jsv_log_info "h_vmem as hard resource requirement has been deleted"
         }
      }
      if {$has_h_data == 1} {
         jsv_sub_del_param "l_hard" "h_data"
         set do_correct 1
         if {[string compare "client" $context] == 0} {
            jsv_log_info "h_data as hard resource requirement has been deleted"
         }
      }
   } 

   if {[jsv_is_param "c"] == 1} {
      set context [jsv_get_param "CONTEXT"]
      set has_c_a [jsv_sub_is_param "c" "a"]
      set has_c_b [jsv_sub_is_param "c" "b"]
   
      if {$has_c_a == 1} {
         set c_a_value [jsv_sub_get_param "c" "a"]
         set new_value [expr $c_a_value + 1]

         jsv_sub_add_param "c" "a" $new_value
      } else {
         jsv_sub_add_param "c" "a" 1
      }
      if {$has_c_b == 1} {
         jsv_sub_del_param "c" "b"
      }
      jsv_sub_add_param "c" "c"
   }

   if {$do_wait == 1} {
      jsv_reject_wait "Job is rejected. It might be submitted later."
   } elseif {$do_correct == 1} {
      jsv_correct "Job was modified before it was accepted"
   } else {
      jsv_accept "Job is accepted"
   }
}

jsv_main

