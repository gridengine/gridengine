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

proc vdep_set_queue_defaults { change_array } {
   upvar $change_array chgar

   set chgar(qtype)                 "BATCH INTERACTIVE"
   set chgar(pe_list)               "NONE"
   set chgar(ckpt_list)             "NONE"
}

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

   foreach host $hostlist {
      puts $CHECK_OUTPUT "modifying queue \"$qname\" for host \"$host\""

      set vi_commands [build_vi_command chgar]
      set cqname "${qname}_${host}"

      set QUEUE [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_OBJ_QUEUE]]
      set NOT_A_QUEUENAME [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_QUEUE_XISNOTAQUEUENAME_S] $cqname ]
      set MODIFIED [translate $ts_config(master_host) 1 0 0 [sge_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS] $CHECK_USER "*" $cqname $QUEUE ]
      set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mq ${cqname}" $vi_commands $MODIFIED $NOT_A_QUEUENAME]
      if { $result == -2 } {
         add_proc_error "set_queue" -1 "$cqname is not a queue"
      }
      if { $result != 0  } {
         add_proc_error "set_queue" -1 "error modify queue $cqname, $result"
      } 
   }

   return $result
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
            set_queue $elem "@all" mod_params 
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
            set_queue $elem "@all" mod_params 
         }
      }
   }
}

proc assign_queues_with_ckpt_object { qname hostlist ckpt_obj } {
   global ts_config
   global CHECK_OUTPUT

   if { $hostlist == "@all" } {
      set hostlist $ts_config(execd_hosts)
   }

   # set ckpt_list in queue object
   foreach host $hostlist {
      set queue "${qname}_${host}"

      get_queue $queue org_val

      if { [string match -nocase "none" $org_val(ckpt_list)] } {
         puts $CHECK_OUTPUT "overwriting NONE value for ckpt_list in queue $queue"
         set new_val(ckpt_list) $ckpt_obj
      } else {
         puts $CHECK_OUTPUT "adding new ckpt object to ckpt_list in queue $queue"
         set new_val(ckpt_list) "$org_val(ckpt_list),$ckpt_obj"
      }

      set_queue $qname $host new_val
   }
}

proc assign_queues_with_pe_object { qname hostlist pe_obj } {
   global ts_config
   global CHECK_OUTPUT

   if { $hostlist == "@all" } {
      set hostlist $ts_config(execd_hosts)
   }

   # set ckpt_list in queue object
   foreach host $hostlist {
      set queue "${qname}_${host}"

      get_queue $queue org_val

      if { [string match -nocase "none" $org_val(pe_list)] } {
         puts $CHECK_OUTPUT "overwriting NONE value for pe_list in queue $queue"
         set new_val(pe_list) $pe_obj
      } else {
         puts $CHECK_OUTPUT "adding new pe object to pe_list in queue $queue"
         set new_val(pe_list) "$org_val(pe_list),$pe_obj"
      }

      set_queue $qname $host new_val
   }
}
