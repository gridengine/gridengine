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

#                                                             max. column:     |
#****** sge_procedures/add_prj() ******
# 
#  NAME
#     add_prj -- ??? 
#
#  SYNOPSIS
#     add_prj { change_array } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     change_array - ??? 
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
#     ???/???
#*******************************
proc add_prj { change_array } {
  global ts_config

# returns 
# -100 on unknown error
# -1   on timeout
# -2   if queue allready exists
# 0    if ok

# name      template
# oticket   0
# fshare    0
# acl       NONE
# xacl      NONE  
# 
  global CHECK_ARCH 
  global CHECK_CORE_MASTER CHECK_USER

  upvar $change_array chgar

  if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     add_proc_error "add_prj" -1 "not possible for sge systems"
     return
  }

  set vi_commands [build_vi_command chgar]

#  set PROJECT [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_PROJECT]]
  set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
  set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]
  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aprj" $vi_commands $ADDED $ALREADY_EXISTS ]
  
  if {$result == -1 } { add_proc_error "add_prj" -1 "timeout error" }
  if {$result == -2 } { add_proc_error "add_prj" -1 "\"[set chgar(name)]\" already exists" }
  if {$result != 0  } { add_proc_error "add_prj" -1 "could not add project \"[set chgar(name)]\"" }

  return $result
}

proc get_prj { prj_name change_array } {
  global ts_config
  global CHECK_ARCH CHECK_OUTPUT

  upvar $change_array chgar

  set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -sprj ${prj_name}" } result ]
  if { $catch_return != 0 } {
     add_proc_error "get_prj" "-1" "qconf error: $result"
     return
  }

  # split each line as listelement
  set help [split $result "\n"]

  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     set value [replace_string $value "{" ""]
     set value [replace_string $value "}" ""]
     
     if { $id != "" } {
        set chgar($id) $value
     }
  }
}

#                                                             max. column:     |
#****** sge_procedures/del_prj() ******
# 
#  NAME
#     del_prj -- ??? 
#
#  SYNOPSIS
#     del_prj { myprj_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     myprj_name - ??? 
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
#     ???/???
#*******************************
proc del_prj { myprj_name } {
  global ts_config
  global CHECK_ARCH open_spawn_buffer CHECK_USER CHECK_CORE_MASTER
  global CHECK_HOST

  if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     set_error -1 "del_prj - not possible for sge systems"
     return
  }

  set REMOVED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS] $CHECK_USER "*" $myprj_name "*" ]

  log_user 0
  set id [ open_remote_spawn_process $CHECK_HOST $CHECK_USER "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-dprj $myprj_name"]
  set sp_id [ lindex $id 1 ]
  set result -1
  set timeout 30 	
  log_user 0 

  expect {
    -i $sp_id full_buffer {
      set result -1
      add_proc_error "del_prj" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
    }
    -i $sp_id "removed" {
      set result 0
    }
    -i $sp_id $REMOVED {
      set result 0
    }

    -i $sp_id default {
      set result -1
    }
  }
  close_spawn_process $id
  log_user 1
  if { $result != 0 } {
     add_proc_error "del_prj" -1 "could not delete project \"$myprj_name\""
  }
  return $result
}

