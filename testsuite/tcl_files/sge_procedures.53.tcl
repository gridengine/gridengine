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


#****** sge_procedures/set_complex() *******************************************
#  NAME
#     set_complex() -- change complex configuration for a complex list
#
#  SYNOPSIS
#     set_complex { change_array complex_list } 
#
#  FUNCTION
#     Set the complex configuration for a specific complex list
#
#  INPUTS
#     change_array - name of an array variable that contains the new complex 
#                    definition
#     complex_list - name of the complex list to change
#
#  RESULT
#     The change_array variable is build as follows:
#
#     set change_array(mem_free) "mf MEMORY 0 <= YES NO 0"   
#   
#     index (mem_free) is the complex name
#     value is "shortcut type value relop requestable consumable default"
#
#  EXAMPLE
#     set mycomplex(load_long) "ll DOUBLE 55.55 >= NO NO 0"
#     set_complex mycomplex host
#
#*******************************************************************************
proc set_complex { change_array complex_list { create 0 } } {
  global ts_config
  global env CHECK_ARCH CHECK_OUTPUT
  global CHECK_CORE_MASTER
  upvar $change_array chgar
  set values [array names chgar]

  if { $create == 0 } {
     get_complex old_values $complex_list
  }

  set vi_commands ""
  foreach elem $values {
     # this will quote any / to \/  (for vi - search and replace)
     set newVal $chgar($elem)
     if {[info exists old_values($elem)]} {
        # if old and new config have the same value, create no vi command,
        # if they differ, add vi command to ...
        if { [compare_complex $old_values($elem) $newVal] != 0 } {
           if { $newVal == "" } {
              # ... delete config entry (replace by comment)
              lappend vi_commands ":%s/^$elem .*$/#/\n"
           } else {
              # ... change config entry
              set newVal1 [split $newVal {/}]
              set newVal [join $newVal1 {\/}]
              lappend vi_commands ":%s/^$elem .*$/$elem  $newVal/\n"
           }
        }
     } else {
        # if the config entry didn't exist in old config: append a new line
        lappend vi_commands "A\n$elem  $newVal[format "%c" 27]"
     }
  } 
  set EDIT_FAILED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PARSE_EDITFAILED]]
  set MODIFIED    [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_MULTIPLY_MODIFIEDIN]]
  set ADDED       [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_MULTIPLY_ADDEDTO]]


  if { $create == 0 } {
     set result [ handle_vi_edit "echo" "\"\"\nSGE_ENABLE_MSG_ID=1\nexport SGE_ENABLE_MSG_ID\n$ts_config(product_root)/bin/$CHECK_ARCH/qconf -mc $complex_list" $vi_commands $MODIFIED $EDIT_FAILED $ADDED ]
  } else {
     set result [ handle_vi_edit "echo" "\"\"\nSGE_ENABLE_MSG_ID=1\nexport SGE_ENABLE_MSG_ID\n$ts_config(product_root)/bin/$CHECK_ARCH/qconf -ac $complex_list" $vi_commands $ADDED $EDIT_FAILED $MODIFIED ]
  }
  if { $result != 0  } {
     add_proc_error "set_complex" -1 "could not modify complex $complex_list ($result)"
  }
  return $result
}

#****** sge_procedures/get_complex() *******************************************
#  NAME
#     get_complex() -- get complex configuration settings
#
#  SYNOPSIS
#     get_complex { change_array complex_list } 
#
#  FUNCTION
#     Get the complex configuration for a specific complex list
#
#  INPUTS
#     change_array - name of an array variable that will get set by 
#                    get_config
#     complex_list - name of a complex list
#
#  RESULT
#     The change_array variable is build as follows:
#
#                      
#     set change_array(mem_free) "mf MEMORY 0 <= YES NO 0"   
#   
#     index (mem_free) is the complex name
#     value is "shortcut type value relop requestable consumable default"
#    
#*******************************************************************************
proc get_complex { change_array complex_list } {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_result [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-sc" "$complex_list"} result ]
  if { $catch_result != 0 } {
     add_proc_error "get_complex" "-1" "qconf error or binary not found ($ts_config(product_root)/bin/$CHECK_ARCH/qconf)\n$result"
     return
  } 

  # split each line as listelement
  set help [split $result "\n"]
  foreach elem $help {
     set id [lindex $elem 0]
     if { [ string first "#" $id ]  != 0 } {
        set value [lrange $elem 1 end]
        if { [string compare $value ""] != 0 } {
           set chgar($id) $value
        }
     }
  }
}



#                                                             max. column:     |
#****** sge_procedures/startup_shadowd() ******
# 
#  NAME
#     startup_shadowd -- ??? 
#
#  SYNOPSIS
#     startup_shadowd { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc startup_shadowd { hostname {env_list ""} } {
  global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER

   if {$env_list != ""} {
      upvar $env_list envlist
   }


   if { $CHECK_ADMIN_USER_SYSTEM == 0 } {  
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_shadowd" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }
 

   puts $CHECK_OUTPUT "starting up shadowd on host \"$hostname\" as user \"$startup_user\""

   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/rcsge" "-shadowd" prg_exit_state 60 0 envlist]
   puts $CHECK_OUTPUT $output
   if { [string first "starting sge_shadowd" $output] >= 0 } {
       return 0
   }
   add_proc_error "startup_shadowd" -1 "could not start shadowd on host $hostname:\noutput:\"$output\""
   return -1
}


#                                                             max. column:     |
#****** sge_procedures/startup_execd() ******
# 
#  NAME
#     startup_execd -- ??? 
#
#  SYNOPSIS
#     startup_execd { hostname } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     hostname - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     sge_procedures/shutdown_core_system()
#     sge_procedures/shutdown_master_and_scheduler()
#     sge_procedures/shutdown_all_shadowd()
#     sge_procedures/shutdown_system_daemon()
#     sge_procedures/startup_qmaster()
#     sge_procedures/startup_execd()
#     sge_procedures/startup_shadowd()
#*******************************
proc startup_execd { hostname } {
  global ts_config
   global CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_ADMIN_USER_SYSTEM CHECK_USER

   if { $CHECK_ADMIN_USER_SYSTEM == 0 } { 
 
      if { [have_root_passwd] != 0  } {
         add_proc_error "startup_execd" "-2" "no root password set or ssh not available"
         return -1
      }
      set startup_user "root"
   } else {
      set startup_user $CHECK_USER
   }

   puts $CHECK_OUTPUT "starting up execd on host \"$hostname\" as user \"$startup_user\""
   set output [start_remote_prog "$hostname" "$startup_user" "$ts_config(product_root)/$ts_config(cell)/common/rcsge" "-execd" prg_exit_state 180]

   set ALREADY_RUNNING [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S] "*"]

   if { [string match "*$ALREADY_RUNNING" $output ] } {
      add_proc_error "startup_execd" -1 "execd on host $hostname is already running"
      return -1
   }

   return 0
}

# ADOC see sge_procedures/get_sge_error_generic()
proc get_sge_error_generic_vdep {messages_var} {
   upvar $messages_var messages

   lappend messages(index) "-100"
   set messages(-100) [translate_macro MSG_SGETEXT_NOQMASTER_PORT_ENV_SI "*" "*"]
   set messages(-100,description) "probably sge_commd and/or sge_qmaster are down"
}
