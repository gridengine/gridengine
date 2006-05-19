#!/usr/local/bin/tclsh
# expect script
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


#****** sge_procedures/assign_queues_with_pe_object() **************************
#  NAME
#     assign_queues_with_pe_object() -- setup queue <-> pe connection
#
#  SYNOPSIS
#     assign_queues_with_pe_object { queue_list pe_obj }
#
#  FUNCTION
#     This procedure will setup the queue - pe connections.
#
#  INPUTS
#     queue_list - queue list for the pe object
#     pe_obj     - name of pe object
#
#  SEE ALSO
#     sge_procedures/assign_queues_with_ckpt_object()
#*******************************************************************************

#                                                             max. column:     |
#****** sge_procedures/add_pe() ******
#
#  NAME
#     add_pe -- add new parallel environment definition object
#
#  SYNOPSIS
#     add_pe { change_array { version_check 1 } }
#
#  FUNCTION
#     This procedure will create a new pe (parallel environemnt) definition
#     object.
#
#  INPUTS
#     change_array - name of an array variable that will be set by add_pe
#     { version_check 1 } - (default 1): check pe/ckpt version
#                           (0): don't check pe/ckpt version
#
#  RESULT
#      0 - ok
#     -1 - timeout error
#     -2 - pe already exists
#     -3 - could not add pe
#
#  EXAMPLE
#     set mype(pe_name) "mype"
#     set mype(user_list) "user1"
#     add_pe pe_name
#
#  NOTES
#     The array should look like this:
#
#     set change_array(pe_name)         "mype"
#     set change_array(user_list)       "crei"
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#
#     pe_name           testpe
#     queue_list        NONE
#     slots             0
#     user_lists        NONE
#     xuser_lists       NONE
#     start_proc_args   /bin/true
#     stop_proc_args    /bin/true
#     allocation_rule   $pe_slots
#     control_slaves    FALSE
#     job_is_first_task TRUE
#
#
#  SEE ALSO
#     sge_procedures/del_pe()
#*******************************
proc add_pe { change_array { version_check 1 } } {
# pe_name           testpe
# queue_list        NONE
# slots             0
# user_lists        NONE
# xuser_lists       NONE
# start_proc_args   /bin/true
# stop_proc_args    /bin/true
# allocation_rule   $pe_slots
# control_slaves    FALSE
# job_is_first_task TRUE

   global ts_config
  global env CHECK_ARCH
  global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

  upvar $change_array chgar

  if { $version_check == 1 } {
     if { $ts_config(gridengine_version) >= 60 && [info exists chgar(queue_list) ]} {
        if { [ info exists chgar(queue_list) ] } {
           puts $CHECK_OUTPUT "this qconf version doesn't support queue_list for pe objects"
           add_proc_error "add_pe" -3 "this qconf version doesn't support queue_list for pe objects,\nuse assign_queues_with_pe_object() after adding pe\nobjects and don't use queue_list parameter.\nyou can call get_pe_ckpt_version() to test pe version"
           unset chgar(queue_list)
        }
     }
  }

  set vi_commands [build_vi_command chgar]

  set ADDED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*" ]
# JG: TODO: have to create separate add_pe in sge_procedures.60.tcl as this message no
#           longer exists
#  set NOT_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNUSERSET_SSSS] "*" "*" "*" "*" ]


#  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-ap [set chgar(pe_name)]" $vi_commands $ADDED $ALREADY_EXISTS $NOT_EXISTS ]
  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-ap [set chgar(pe_name)]" $vi_commands $ADDED $ALREADY_EXISTS ]

  if {$result == -1 } { add_proc_error "add_pe" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_pe" -1 "parallel environment \"[set chgar(pe_name)]\" already exists" }
  if {$result == -3 } { add_proc_error "add_pe" -1 "something (perhaps a queue) does not exist" }
  if {$result != 0  } { add_proc_error "add_pe" -1 "could not add parallel environment \"[set chgar(pe_name)]\"" }

  return $result
}

proc get_pe {pe_name change_array} {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sp" "$pe_name"} result ]
  if { $catch_result != 0 } {
     add_proc_error "get_config" "-1" "qconf error or binary not found ($ts_config(product_root)/bin/$CHECK_ARCH/qconf)\n$result"
     return
  }

  # split each line as listelement
  set help [split $result "\n"]
  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     if { [string compare $value ""] != 0 } {
       set chgar($id) $value
     }
  }
}

#****** sge_procedures/set_pe() ************************************************
#  NAME
#     set_pe() -- set or change pe configuration
#
#  SYNOPSIS
#     set_pe { pe_obj change_array }
#
#  FUNCTION
#     Set a pe configuration corresponding to the content of the change_array.
#
#  INPUTS
#     pe_obj         - name of the pe.
#     change_array   - TCL array specifying the pe object to configure
#                      The following array members can be set:
#                        - slots             optional, default 0
#                        - user_lists        optional, default NONE
#                        - xuser_lists       optional, default NONE
#                        - start_proc_args   optional, default /bin/true
#                        - stop_proc_args    optional, default /bin/true
#                        - allocation_rule   optional, default $pe_slots
#                        - control_slaves    optional, default FALSE
#                        - job_is_first_task optional, default TRUE
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  SEE ALSO
#     sge_procedures/add_pe()
#*******************************************************************************
proc set_pe {pe_obj change_array} {

   global ts_config
   global env CHECK_ARCH
   global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

   upvar $change_array chgar

   if {$ts_config(gridengine_version) >= 60 && [info exists chgar(queue_list)]} {
      if {[info exists chgar(queue_list)]} {
         add_proc_error "set_pe" -3 "this qconf version doesn't support queue_list for pe objects,\nuse assign_queues_with_pe_object() after adding pe\nobjects and don't use queue_list parameter.\nyou can call get_pe_ckpt_version() to test pe version"
         unset chgar(queue_list)
      }
   }

   set vi_commands [build_vi_command chgar]

   set MODIFIED  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" "*" "*"]
   set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*" ]

   if { $ts_config(gridengine_version) == 53 } {
      set NOT_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_UNKNOWNUSERSET_SSSS] "*" "*" "*" "*" ]
   } else {
      # JG: TODO: is it the right message? It's the only one mentioning non 
      #           existing userset, but only for CQUEUE?
      set NOT_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_CQUEUE_UNKNOWNUSERSET_S] "*" ]
   }

   set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mp $pe_obj" $vi_commands $MODIFIED $ALREADY_EXISTS $NOT_EXISTS ]

   if {$result == -1 } { add_proc_error "set_pe" -1 "timeout error" }
   if {$result == -2 } { add_proc_error "set_pe" -1 "parallel environment \"$pe_obj\" already exists" }
   if {$result == -3 } { add_proc_error "set_pe" -1 "something (perhaps a queue) does not exist" }
   if {$result != 0  } { add_proc_error "set_pe" -1 "could not change parallel environment \"$pe_obj\"" }

   return $result
}

#                                                             max. column:     |
#****** sge_procedures/del_pe() ******
#
#  NAME
#     del_pe -- delete parallel environment object definition
#
#  SYNOPSIS
#     del_pe { pe_name }
#
#  FUNCTION
#     This procedure will delete a existing parallel environment, defined with
#     sge_procedures/add_pe.
#
#  INPUTS
#     mype_name - name of parallel environment to delete
#
#  RESULT
#     0  - ok
#     <0 - error
#
#  SEE ALSO
#     sge_procedures/add_pe()
#*******************************
proc del_pe {pe_name} {
   global ts_config
   global CHECK_USER

   unassign_queues_with_pe_object $pe_name

   set messages(index) "0"
   set messages(0) [translate_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS $CHECK_USER "*" $pe_name "*"]

   set output [start_sge_bin "qconf" "-dp $pe_name"]

   set ret [handle_sge_errors "del_pe" "qconf -dp $pe_name" $output messages]
   return $ret
}

