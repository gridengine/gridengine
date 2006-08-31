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

proc get_queue_instance {queue host} {
   set resolved_host [resolve_host $host 1]
   return "${queue}@${resolved_host}"
}

#****** sge_procedures.60/queue/vdep_set_queue_defaults() **********************
#  NAME
#     vdep_set_queue_defaults() -- create version dependent queue settings
#
#  SYNOPSIS
#     vdep_set_queue_defaults { change_array } 
#
#  FUNCTION
#     Fills the array change_array with queue attributes specific for SGE 6.0
#
#  INPUTS
#     change_array - the resulting array
#
#  SEE ALSO
#     sge_procedures/queue/set_queue_defaults()
#*******************************************************************************
proc vdep_set_queue_defaults { change_array } {
   upvar $change_array chgar

   set chgar(hostlist)              "hostlist"
   set chgar(qtype)                 "BATCH INTERACTIVE"
   set chgar(pe_list)               "NONE"
   set chgar(ckpt_list)             "NONE"
}

#****** sge_procedures.60/queue/validate_queue() ********************************
#  NAME
#     validate_queue() -- validate the settings for queue_type
#
#  SYNOPSIS
#     validate_queue { change_array } 
#
#  FUNCTION
#     Removes the queue types PARALLEL and CHECKPOINTING from the queue_types
#     attribute in change_array.
#     These attributes are implicitly set in SGE 6.0 by setting the attributes
#     ckpt_list and pe_list.
#
#     Sets a cluster dependent tmpdir to avoid problems when running multiple
#     testsuite clusters in parallel shareing hosts.
#
#  INPUTS
#     change_array - array containing queue definitions
#
#*******************************************************************************
proc validate_queue { change_array } {
   global ts_config CHECK_OUTPUT

   upvar $change_array chgar

   if {[info exists chgar(qtype)]} {
      if { [string match "*CHECKPOINTING*" $chgar(qtype)] ||
           [string match "*PARALLEL*" $chgar(qtype)] } { 

         set new_chgar_qtype ""
         foreach elem $chgar(qtype) {
            if { [string match "*CHECKPOINTING*" $elem] } {
               puts $CHECK_OUTPUT "queue type CHECKPOINTING is set by assigning a checkpointing environment to the queue"
            } else {
               if { [string match "*PARALLEL*" $elem] } {
                  puts $CHECK_OUTPUT "queue type PARALLEL is set by assigning a parallel environment to the queue"
               } else {
                  append new_chgar_qtype "$elem "
               }
            }
         }
         set chgar(qtype) [string trim $new_chgar_qtype]
         puts $CHECK_OUTPUT "using qtype=$chgar(qtype)" 
      }
   }

   # create cluster dependent tmpdir
   set chgar(tmpdir) "/tmp/testsuite_$ts_config(commd_port)"
}

proc qinstance_to_cqueue { change_array } {
   global CHECK_OUTPUT

   upvar $change_array chgar

   if { [info exists chgar(hostname)] } {
      unset chgar(hostname)
   }

}

#****** sge_procedures.60/queue/add_queue() ******************************************
#  NAME
#     add_queue() -- add a SGE 6.0 cluster queue
#
#  SYNOPSIS
#     add_queue { qname hostlist change_array {fast_add 1} } 
#
#  FUNCTION
#     Adds a cluster queues to a SGE 6.0 system.
#
#  INPUTS
#     qname        - name for the (cluster) queue
#     hostlist     - list of hostnames or names of host groups
#     change_array - array containing attributes that differ from defaults
#     {fast_add 1} - 0: add the queue using qconf -aq,
#                    1: add the queue using qconf -Aq, much faster!
#
#  RESULT
#     integer value  0 on success, -2 on error
#
#*******************************************************************************
proc add_queue { qname hostlist change_array {fast_add 1} } {
   global ts_config
   global CHECK_ARCH CHECK_OUTPUT CHECK_USER

   upvar $change_array chgar

   # queue_type is version dependent
   validate_queue chgar

   puts $CHECK_OUTPUT "creating queue \"$qname\" for hostlist \"$hostlist\""

   set chgar(qname)     "$qname"
   set chgar(hostlist)  "$hostlist"

   # localize messages
   # JG: TODO: object name is taken from c_gdi object structure - not I18Ned!!
   set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
   set ALREADY_EXISTS [ translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "cluster queue" $qname]
   set ADDED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" $qname "cluster queue" ]

   # add queue from file?
   if { $fast_add } {
      set_queue_defaults default_array
      update_change_array default_array chgar

      set tmpfile [dump_array_to_tmpfile default_array]

      set result [start_sge_bin "qconf" "-Aq ${tmpfile}"]
      puts $CHECK_OUTPUT $result

      if { [string match "*$ADDED*" $result ] } {
         set result 0
      } else {
         add_proc_error "add_queue" "-1" "qconf error or binary not found"
         set result -2
      }
   } else {
      # add by handling vi
      set vi_commands [build_vi_command chgar]

      set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aq" $vi_commands $ADDED $ALREADY_EXISTS ]  
      if { $result != 0 } {
         add_proc_error "add_queue" -1 "could not add queue [set chgar(qname)] (error: $result)"
      }
   }

   return $result
}

# set_queue_work - no public interface
proc set_queue_work { qname change_array {raise_error 1} } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH CHECK_USER

   upvar $change_array chgar

   puts $CHECK_OUTPUT "modifying queue \"$qname\""

   set vi_commands [build_vi_command chgar]

   # JG: TODO: object name is taken from c_gdi object structure - not I18Ned!!
   set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
   set MODIFIED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $qname "cluster queue" ]
   set UNKNOWN_ATTRIBUTE [ translate_macro MSG_UNKNOWNATTRIBUTENAME_S $qname ]
   set NOT_MODIFIED [translate_macro MSG_FILE_NOTCHANGED ]

   #set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mq ${qname}" $vi_commands $MODIFIED]
   set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mq ${qname}" $vi_commands $MODIFIED $NOT_MODIFIED $UNKNOWN_ATTRIBUTE ]
   if { $result == -2 } {
      add_proc_error "set_queue_work" -1 "queue $qname not modified" $raise_error
   }
   if { $result == -3 } {
      add_proc_error "set_queue_work" -1 "unknown attribute in queue $qname " $raise_error
   }
   if { $result != 0  } {
      add_proc_error "set_queue_work" -1 "error modify queue $qname, $result" $raise_error
   } 

   return $result
}

proc set_cqueue_default_values { current_array change_array } {
   global CHECK_OUTPUT

   upvar $current_array currar
   upvar $change_array chgar
      puts $CHECK_OUTPUT "calling set_cqueue_default_values"

   # parse each attribute to be changed and set the queue default value
   foreach attribute [array names chgar] {
      puts $CHECK_OUTPUT "--> setting queue default value for attribute $attribute"
      puts $CHECK_OUTPUT "--> old_value = $currar($attribute)"
      # set the default
      set new_value $chgar($attribute)
      puts $CHECK_OUTPUT "--> new_value = $new_value"

      # get position of host(group) specific values and append them 
      set comma_pos [string first ",\[" $currar($attribute)]
      puts $CHECK_OUTPUT "--> comma pos = $comma_pos"
      if { $comma_pos != -1 } {
         append new_value [string range $currar($attribute) $comma_pos end]
      }

      puts $CHECK_OUTPUT "--> new queue default value = $new_value"
      # write back to chgar
      set chgar($attribute) $new_value
   }
}

proc set_cqueue_specific_values { current_array change_array hostlist } {
   global CHECK_OUTPUT

   upvar $current_array currar
   upvar $change_array chgar
      puts $CHECK_OUTPUT "calling set_cqueue_specific_values"

   # parse each attribute to be changed
   foreach attribute [array names chgar] {
      if { [string compare $attribute qname] == 0 } {
         continue;
      }

      puts $CHECK_OUTPUT "--> setting queue default value for attribute $attribute"
      puts $CHECK_OUTPUT "--> old_value = $currar($attribute)"
     
      # split old value and store host specific values in an array
      if { [info exists host_values] } {
         unset host_values
      }

      # split attribute value into default and host specific components
      set value_list [split $currar($attribute) "\["]

      # copy the default value
      if { $hostlist == "" } {
         # use the new value for the cluster queue
         set new_value $default_value
      } else {
         # use old cqueue value as default, set new host specific
         set default_value [string trimright [lindex $value_list 0] ","]
         puts $CHECK_OUTPUT "--> default value = $default_value"

         # copy host specific values to array
         for {set i 1} {$i < [llength $value_list]} {incr i} {
            set host_value [lindex $value_list $i]
            set first_equal_position [string first "=" $host_value]
            incr first_equal_position -1
            set host  [string range $host_value 0 $first_equal_position]
            incr first_equal_position 2
            set value [string range $host_value $first_equal_position end]
            set value [string trimright $value ",\]\\"]
            puts $CHECK_OUTPUT "--> \"$host\" = \"$value\""
            set host_values($host) $value
         }
      
         # change (or set) host specific values from chgar
         foreach unresolved_host $hostlist {
            set host [resolve_host $unresolved_host 1]
            puts $CHECK_OUTPUT "--> setting host_values($host) = $chgar($attribute)"
            set host_values($host) $chgar($attribute)
         }

         # dump host specific values to new_value
         set new_value $default_value
         foreach host [array names host_values] {
            if {[string compare -nocase $default_value $host_values($host)] != 0} {
               append new_value ",\[$host=$host_values($host)\]"
            }
         }
      }

      puts $CHECK_OUTPUT "--> new queue value = $new_value"

      # write back to chgar
      set chgar($attribute) $new_value
   }

   # check if all hosts / hostgroups are in the hostlist attribute
#   if { $hostlist != "" } {
#      set new_hosts {}
#      foreach host $hostlist {
#         if { [lsearch -exact $currar(hostlist) $host] == -1 } {
#            lappend new_hosts $host
#            puts $CHECK_OUTPUT "--> host $host is not yet in hostlist"
#         }
#      }
#
#      if { [llength $new_hosts] > 0 } {
#         set chgar(hostlist) "$currar(hostlist) $new_hosts"
#      }
#   }
}

#****** sge_procedures.60/queue/set_queue() ******************************************
#  NAME
#     set_queue() -- set queue attributes
#
#  SYNOPSIS
#     set_queue { qname hostlist change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} 
#
#  FUNCTION
#     Sets the attributes given in change_array in the cluster queue qname.
#     If hostlist is an empty list, the cluster queue global values are set.
#     If a list of hosts or host groups is specified, the attributes for these
#     hosts or host groups are set.
#
#  INPUTS
#     qname        - name of the (cluster) queue
#     hostlist     - list of hosts / host groups. 
#     change_array - array containing the changed attributes.
#     {fast_add 1} - 0: modify the attribute using qconf -mq,
#                  - 1: modify the attribute using qconf -Mq, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#
#*******************************************************************************
proc set_queue { qname hostlist change_array {fast_add 1}  {on_host ""} {as_user ""}  {raise_error 1}} {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   upvar $change_array chgar

   # queue_type is version dependent
   validate_queue chgar
   qinstance_to_cqueue chgar

   get_queue $qname currar

   # process chgar and set values
   if { [llength $hostlist] == 0 } {
      set_cqueue_default_values currar chgar
   } else {
      set_cqueue_specific_values currar chgar $hostlist
   }

   # Modify queue from file
   if { $fast_add } {

     # Add in currar elements from chgar which have NOT
     # been changed
     
     foreach elem [array names chgar] {
        set fastar($elem) $chgar($elem)
     }
     foreach elem [array names currar] {
        if { ![info exists chgar($elem)] } {
           set fastar($elem) $currar($elem)
        }
     }
 
     set tmpfile [dump_array_to_tmpfile fastar]
     set ret [start_sge_bin "qconf" "-Mq $tmpfile" $on_host $as_user ]
     
     if {$prg_exit_state == 0} {
         set result 0
      } else {
         set result [mod_queue_error $ret $qname $tmpfile $raise_error]
      }

   } else {

      # do the work
      set result [set_queue_work $qname chgar $raise_error]
      
      if { $result != 0 } {
            add_proc_error "set_queue" -1 "could not modifiy queue $qname " $raise_error
            set result -1
      }

   }
   
   return $result
}

#****** sge_queue.60/mod_queue_error() ***************************************
#  NAME
#     mod_queue_error() -- error handling for mod_queue/set_queue
#
#  SYNOPSIS
#     mod_queue_error {result qname tmpfile raise_error }
#
#  FUNCTION
#     Does the error handling for mod_queue/set_queue.
#     Translates possible error messages of qconf -Mq,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_queue/set_queue. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     qname       - queue name for qconf -Mq
#     tmpfile     - temp file for qconf -Mq
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_queue/set_queue function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#******************************************************************************
proc mod_queue_error {result qname tmpfile raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2"
   set messages(-1) [translate_macro MSG_OBJECT_VALUENOTULONG_S "*" ]
   set messages(-2) [translate_macro MSG_SGETEXT_DOESNOTEXIST_SS "cluster queue" $qname]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_queue_error" "qconf -Mq $tmpfile" $result messages $raise_error]

   return $ret
}

#****** sge_queue.60/queue/mod_queue() ******************************************
#  NAME
#     mod_queue() -- wrapper for set_queue 
#
#  SYNOPSIS
#     mod_queue { qname hostlist change_array {fast_add 1} {on_host ""} {as_user ""} {raise_errori 1}}
#
#  FUNCTION
#     See set_queue
#
#  INPUTS
#     qname        - name of the (cluster) queue
#     hostlist     - list of hosts / host groups.
#     change_array - array containing the changed attributes.
#     {fast_add 1} - 0: modify the attribute using qconf -mckpt,
#                  - 1: modify the attribute using qconf -Mckpt, faster
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - do add_proc_error in case of errors
#
#  RESULT
#
#*******************************************************************************

proc mod_queue { qname hostlist change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $change_array chgar
   return [set_queue $qname $hostlist chgar $fast_add $on_host $as_user $raise_error]
}

#####

proc del_queue { q_name hostlist {ignore_hostlist 0} {del_cqueue 0}} {
  global ts_config
  global CHECK_ARCH CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

   if {!$ignore_hostlist} {
      # delete individual queue instances or queue domaines
      foreach host $hostlist {
         set result ""
         set catch_return [ catch {
            eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -dattr queue hostlist $host $q_name" 
            } result ]
         if { $catch_return != 0 } {
            add_proc_error "del_queue" "-1" "could not delete queue instance or queue domain: $result"
         }
      }
   }

   if {$ignore_hostlist || $del_cqueue} {
      set result ""
      set catch_return [ catch {  
         eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -dq ${q_name}" 
      } result ]

      # JG: TODO: object name is taken from c_gdi object structure, not I18Ned!!
      set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
      set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $q_name "cluster queue" ]
   
      if { [string match "*$REMOVED" $result ] == 0 } {
         add_proc_error "del_queue" "-1" "could not delete queue $q_name: (error: $result)"
         return -1
      } 
   }
   return 0
}

proc get_queue_list {} {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   set NO_QUEUE_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "cqueue list"]

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

proc get_qinstance_list {{filter ""}} {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   set NO_QUEUE_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QSTAT_NOQUEUESREMAININGAFTERXQUEUESELECTION_S] "-pe"]

   # try to get qinstance list
   if { $filter != "" } {
      set arg1 [lindex $filter 0]
      set arg2 [lindex $filter 1]
      set ret [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qselect" "$arg1" "$arg2"} result]
   } else {
      set ret [catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qselect" } result]
   }
   if { $ret != 0 } {
      # command failed because queue list is empty
      if { [string compare $NO_QUEUE_DEFINED $result] == 0 } {
         puts $CHECK_OUTPUT $result
         set result {}
      } else {
         # if command fails: output error
         add_proc_error "get_qinstance_list" -1 "error reading queue list: $result"
         set result {}
      }
   }

   return $result
}

# queue for -q request or as subordinate queue
# is the 6.0 cluster queue
proc get_requestable_queue { queue host } {
   return $queue
}

proc get_cluster_queue {queue_instance} {
   set cqueue $queue_instance

   if {$queue_instance != "" } {
      set at [string first "@" $queue_instance]
      if {$at > 0} {
         set cqueue [string range $queue_instance 0 [expr $at - 1]]
      }
   }

   puts $CHECK_OUTPUT "queue instance $queue_instance is cluster queue $cqueue"

   return $cqueue
}
proc get_clear_queue_error_vdep {messages_var host} {
   upvar $messages_var messages

   #lappend messages(index) "-3"
   #set messages(-3) [translate_macro MSG_XYZ_S $host] #; another exechost specific error message
   #set messages(-3,description) "a highlevel description of the error"    ;# optional parameter
   #set messages(-3,level) -2  ;# optional parameter: we only want to raise a warning
}

#****** sge_queue.60/purge_queue() *****************************************
#  NAME
#     purge_queue() -- purge queue object queue@$host
#
#  SYNOPSIS
#     purge_queue { queue host object {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -purge queue hostlist $queue@$host to clear purge $queue
#
#  INPUTS
#     queue           - queue to be cleared
#     host            - host instnace on which to purge queue
#     object          - object to be purged: hostlist, load_threashold,...
#     output_var      - result will be placed here
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_calendar/get_calendar()
#     sge_calendar/get_calendar_error()
#*******************************************************************************
proc purge_queue {queue host object {output_var result}  {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-purge queue $object $queue@$host" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [purge_queue_error $result $queue $host $object $raise_error]
   }

   return $ret

}
#****** sge_queue.60/purge_queue_error() ***************************************
#  NAME
#     purge_queue_error() -- error handling for purge_queue
#
#  SYNOPSIS
#     purge_queue_error { result queue host object raise_error }
#
#  FUNCTION
#     Does the error handling for purge_queue.
#     Translates possible error messages of qconf -purge,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     purge_queue. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     queue       - queue for which qconf -purge has been called
#     host        - host on which queue will be purged
#     object      - object  which queue will be purged
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for purge_queue function:
#      -1:  Cluster queue entry "queue" does not exist
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc purge_queue_error {result queue host object raise_error} {

   global CHECK_OUTPUT

   # recognize certain error messages and return special return code
   set messages(index) "-1 "
   set messages(-1) [concat "error: " [translate_macro MSG_CQUEUE_DOESNOTEXIST_S $queue]]

   # we might have version dependent, calendar specific error messages
   get_clear_queue_error_vdep messages $queue

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "purge_queue" "qconf -purge $object $queue@$host" $result messages $raise_error]

   return $ret
}

