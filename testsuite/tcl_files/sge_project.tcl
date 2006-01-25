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
#****** sge_project/del_prj() ******
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
proc del_prj { prj_name } {
   global ts_config
   global CHECK_USER

   if {$ts_config(product_type) == "sge"} {
      set_error -1 "del_prj - not possible for sge systems"
      return -1
   }

   set messages(index) "0"
   set messages(0) [translate_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS $CHECK_USER "*" $myprj_name "*"]

   set output [start_sge_bin "qconf" "-dprj $prj_name"]

   set ret [handle_sge_errors "del_prj" "qconf -dprj $prj_name" $output messages]
   return $ret
}

#****** sge_project/get_project_list() *****************************************
#  NAME
#    get_project_list () -- get the list of projects
#
#  SYNOPSIS
#     get_project_list { {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sprjl to retrieve the project list
#
#  INPUTS
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
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************************************************************
proc get_project_list {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_project_list" "-sprjl" out $on_host $as_user $raise_error]

}

