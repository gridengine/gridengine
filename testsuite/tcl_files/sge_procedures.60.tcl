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

#****** sge_procedures.60/queue/vdep_set_queue_defaults() ****************************
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

#****** sge_procedures.60/queue/validate_queue_type() ********************************
#  NAME
#     validate_queue_type() -- validate the settings for queue_type
#
#  SYNOPSIS
#     validate_queue_type { change_array } 
#
#  FUNCTION
#     Removes the queue types PARALLEL and CHECKPOINTING from the queue_types
#     attribute in change_array.
#     These attributes are implicitly set in SGE 6.0 by setting the attributes
#     ckpt_list and pe_list.
#
#  INPUTS
#     change_array - array containing queue definitions
#
#*******************************************************************************
proc validate_queue_type { change_array } {
   global CHECK_OUTPUT

   upvar $change_array chgar

   if { [info exists chgar(qtype)] } {
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
}

#****** sge_procedures.60/queue/add_queue() ******************************************
#  NAME
#     add_queue() -- add a SGE 6.0 cluster queue
#
#  SYNOPSIS
#     add_queue { qname hostlist change_array {fast_add 0} } 
#
#  FUNCTION
#     Adds a cluster queues to a SGE 6.0 system.
#
#  INPUTS
#     qname        - name for the (cluster) queue
#     hostlist     - list of hostnames or names of host groups
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

      set result ""
      set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -Aq ${tmpfile}" } result ]
      puts $CHECK_OUTPUT $result

      if { [string match "*$ADDED" $result ] == 0 } {
         add_proc_error "add_queue" "-1" "qconf error or binary not found"
      } 
   } else {
      # add by handling vi
      set vi_commands [build_vi_command chgar]

      set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aq" $vi_commands $ADDED $ALREADY_EXISTS ]  
      if { $result != 0 } {
         add_proc_error "add_queue" -1 "could not add queue [set chgar(qname)] (error: $result)"
         break
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

   # JG: TODO: object name is taken from c_gdi object structure - not I18Ned!!
   set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
   set MODIFIED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $qname "cluster queue" ]
   set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mq ${qname}" $vi_commands $MODIFIED]
   if { $result == -2 } {
      add_proc_error "set_queue" -1 "$qname is not a queue"
   }
   if { $result != 0  } {
      add_proc_error "set_queue" -1 "error modify queue $qname, $result"
   } 

   return $result
}

proc set_cqueue_default_values { current_array change_array } {
   global CHECK_OUTPUT

   upvar $current_array currar
   upvar $change_array chgar

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
         append new_value [string range $comma_pos end]
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

   # parse each attribute to be changed
   foreach attribute [array names chgar] {
      puts $CHECK_OUTPUT "--> setting queue default value for attribute $attribute"
      puts $CHECK_OUTPUT "--> old_value = $currar($attribute)"
     
      # split old value and store host specific values in an array
      if { [info exists host_values] } {
         unset host_values
      }

      # split attribute value into default and host specific components
      set value_list [split $currar($attribute) "\["]

      # copy the default value
      set new_value [string trimright [lindex $value_list 0] ","]
      puts $CHECK_OUTPUT "--> default value = $new_value"
   
      # copy host specific values to array
      for {set i 1} {$i < [llength $value_list]} {incr i} {
         set host_value [lindex $value_list $i]
         set split_host_value [split $host_value "="]
         set host [lindex $split_host_value 0]
         set value [lrange $split_host_value 1 end]
         set value [string trimright $value ",\]"]
         puts $CHECK_OUTPUT "--> $host = $value"
         set host_values($host) $value
      }
   
      # change (or set) host specific values from chgar
      foreach host $hostlist {
         set host_values($host) $chgar($attribute)
      }

      # dump host specific values to new_value
      foreach host [array names host_values] {
         append new_value ",\[$host=$host_values($host)\]"
      }

      puts $CHECK_OUTPUT "--> new queue value = $new_value"

      # write back to chgar
      set chgar($attribute) $new_value
   }

   # check if all hosts / hostgroups are in the hostlist attribute
   set new_hosts {}
   foreach host $hostlist {
      if { [lsearch -exact $currar(hostlist) $host] == -1 } {
         lappend new_hosts $host
         puts $CHECK_OUTPUT "--> host $host is not yet in hostlist"
      }
   }

   if { [llength $new_hosts] > 0 } {
      set chgar(hostlist) "$currar(hostlist) $new_hosts"
   }
}

#****** sge_procedures.60/queue/set_queue() ******************************************
#  NAME
#     set_queue() -- set queue attributes
#
#  SYNOPSIS
#     set_queue { qname hostlist change_array } 
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

   get_queue $qname currar

   # process chgar and set values
   if { [llength $hostlist] == 0 } {
      set_cqueue_default_values currar chgar
   } else {
      set_cqueue_specific_values currar chgar $hostlist
   }

   # do the work
   set result [set_queue_work $qname chgar]

   return $result
}

proc del_queue { q_name } {
  global ts_config
  global CHECK_ARCH open_spawn_buffer CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT CHECK_HOST

  set result ""
  set catch_return [ catch {  
      eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -dq ${q_name}" 
  } result ]

  # JG: TODO: object name is taken from c_gdi object structure - not I18Ned!!
  set QUEUE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $q_name "cluster queue" ]

  if { [string match "*$REMOVED" $result ] == 0 } {
     add_proc_error "del_queue" "-1" "could not delete queue $q_name: (error: $result)"
     return -1
  } 
  return 0
}

proc unassign_queues_with_pe_object { pe_obj } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   puts $CHECK_OUTPUT "searching for references in all defined queues ..."
   set NO_QUEUE_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "queue"]
   catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sql" } result
   if { [string first $NO_QUEUE_DEFINED $result] >= 0 } {
      puts $CHECK_OUTPUT "no queue defined"
   } else {
      foreach elem $result {
         puts $CHECK_OUTPUT "queue: $elem"
         if {[info exists params]} {
            unset params
         }

         get_queue $elem params
         if { [string first $pe_obj $params(pe_list)] >= 0 } {
            puts $CHECK_OUTPUT "pe obj $pe_obj is referenced in queue $elem, removing entry."
            set new_params ""
            set help_list [split $params(pe_list) ","]
            set help_list2 ""
            foreach element $help_list {
               append help_list2 "$element "
            }

            set help_list [string trim $help_list2] 
            foreach help_pe $help_list {
               if { [string match $pe_obj $help_pe] != 1 } {
                  append new_params "$help_pe "
               }
            }

            if { [llength $new_params ] == 0 } {
               set new_params "NONE"
            }

            set mod_params(pe_list) [string trim $new_params]
            set_queue $elem "" mod_params 
         }
      }
   }
}

proc unassign_queues_with_ckpt_object { ckpt_obj } {
   global ts_config
   global CHECK_OUTPUT CHECK_ARCH

   puts $CHECK_OUTPUT "searching for references in all defined queues ..."
   set NO_QUEUE_DEFINED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QCONF_NOXDEFINED_S] "queue"]
   catch { exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sql" } result
   if { [string first $NO_QUEUE_DEFINED $result] >= 0 } {
      puts $CHECK_OUTPUT "no queue defined"
   } else {
      foreach elem $result {
         puts $CHECK_OUTPUT "queue: $elem"
         if {[info exists params]} {
            unset params
         }

         get_queue $elem params
         if { [string first $ckpt_obj $params(ckpt_list)] >= 0 } {
            puts $CHECK_OUTPUT "ckpt obj $ckpt_obj is referenced in queue $elem, removing entry."
            set new_params ""
            set help_list [split $params(ckpt_list) ","]
            set help_list2 ""
            foreach element $help_list {
               append help_list2 "$element "
            }

            set help_list [string trim $help_list2] 
            foreach help_ckpt $help_list {
               if { [string match $ckpt_obj $help_ckpt] != 1 } {
                  append new_params "$help_ckpt "
               }
            }

            if { [llength $new_params ] == 0 } {
               set new_params "NONE"
            }

            set mod_params(ckpt_list) [string trim $new_params]
            set_queue $elem "" mod_params 
         }
      }
   }
}

proc assign_queues_with_ckpt_object { qname hostlist ckpt_obj } {
   global CHECK_OUTPUT

   get_queue $qname org_val

   # JG: TODO: we need functions get_default_value and get_specific_value(host)
   #           in the meantime, the hostlist parameter will be ignored!
   if { [string match -nocase "none" $org_val(ckpt_list)] } {
      puts $CHECK_OUTPUT "overwriting NONE value for ckpt_list in queue $qname"
      set new_val(ckpt_list) $ckpt_obj
   } else {
      puts $CHECK_OUTPUT "adding new ckpt object to ckpt_list in queue $qname"
      set new_val(ckpt_list) "$org_val(ckpt_list),$ckpt_obj"
   }

   set_queue $qname $hostlist new_val
}

proc assign_queues_with_pe_object { qname hostlist pe_obj } {
   global CHECK_OUTPUT

   get_queue $qname org_val

   if { [string match -nocase "none" $org_val(pe_list)] } {
      puts $CHECK_OUTPUT "overwriting NONE value for pe_list in queue $qname"
      set new_val(pe_list) $pe_obj
   } else {
      puts $CHECK_OUTPUT "adding new pe object to pe_list in queue $qname"
      set new_val(pe_list) "$org_val(pe_list),$pe_obj"
   }

   set_queue $qname $hostlist new_val
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
