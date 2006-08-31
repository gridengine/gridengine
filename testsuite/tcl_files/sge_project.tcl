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

proc set_project_defaults {change_array} {
   global ts_config CHECK_OUTPUT

   upvar $change_array prj

   set prj(name) template
   set prj(oticket) 0
   set prj(fshare) 0
   set prj(acl) NONE
   set prj(xacl) NONE
}

#                                                             max. column:     |
#****** sge_procedures/add_prj() ******
# 
#  NAME
#     add_prj -- add a project
#
#  SYNOPSIS
#     add_prj {change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}}
#
#  FUNCTION
#     Add a project to the Grid Engine cluster.
#     Supports fast (qconf -Aprj) and slow (qconf -aprj) mode.
#
#  INPUTS
#     change_array    - the project description
#     {fast_add 1}    -  use fast mode
#     {on_host ""}    - execute qconf on this host (default: qmaster host)
#     {as_user ""}    - execute qconf as this user (default: CHECK_USER)
#     {raise_error 1} - raise error condition in case of errors?
#
#  RESULT
#       0 - success
#     < 0 - error:
#           -1 : timeout error (in editor called by qconf -aprj)
#           -2 : project already exists
#           -9 : we are in sge mode where we don't have projects
#           ...: other errors, see sge_procedures/get_sge_error()
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#*******************************************************************************
proc add_prj {change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT
   global CHECK_ARCH 

   upvar $change_array chgar

   if {[string compare $ts_config(product_type) "sge"] == 0} {
      add_proc_error "add_prj" -1 "not possible for sge systems"
      return -9
   }

   set PROJECT [translate_macro MSG_PROJECT]
   set ADDED [translate_macro MSG_SGETEXT_ADDEDTOLIST_SSSS "*" "*" "*" $PROJECT]
   set ALREADY_EXISTS [translate_macro MSG_SGETEXT_ALREADYEXISTS_SS "*" "*"]

   if {$fast_add} {
      set_project_defaults old_config
      foreach elem [array names chgar] {
         set old_config($elem) $chgar($elem)
      }
      set tmpfile [dump_array_to_tmpfile old_config]
      set result [start_sge_bin "qconf" "-Aprj $tmpfile" $on_host $as_user]
      set messages(index) {0 -2}
      set messages(0) $ADDED
      set messages(-2) $ALREADY_EXISTS
      set ret [handle_sge_errors "add_prj" "qconf -Aprj" $result messages $raise_error]
   } else {
      set vi_commands [build_vi_command chgar]

      set ret [handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-aprj" $vi_commands $ADDED $ALREADY_EXISTS]
  
      if {$ret == -1} { add_proc_error "add_prj" -1 "timeout error" $raise_error }
      if {$ret == -2} { add_proc_error "add_prj" -1 "project \"$chgar(name)\" already exists" $raise_error }
      if {$ret != 0 } { add_proc_error "add_prj" -1 "could not add project \"$chgar(name)\"" $raise_error }
   }

   return $ret
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
   set messages(0) [translate_macro MSG_SGETEXT_REMOVEDFROMLIST_SSSS $CHECK_USER "*" $prj_name "*"]

   set output [start_sge_bin "qconf" "-dprj $prj_name"]

   set ret [handle_sge_errors "del_prj" "qconf -dprj $prj_name" $output messages]
   return $ret
}
#                                                             max. column:     |
#****** sge_project/del_project() ******
#
#  NAME
#     del_project -- wrapper for del_prj
#
#  SYNOPSIS
#    del_project{ myprj_name }
#
#  FUNCTION
#     Wrapper for del_prj
#
#  INPUTS
#     myprj_name - project
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
proc del_project { myprj_name } {

     return [del_prj $myprj_name]
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
#                                                             max. column:     |
#****** sge_project/mod_project() ******
#
#  NAME
#     mod_project -- Modify project
#
#  SYNOPSIS
#     mod_project {attribute value change_array {fast_add 1} {on_host ""} {as_user ""} raise_error}
#
# returns
# -100 on unknown error from handle_vi_edit
# -1   on timeout
# -2   if queue already exists
# -3   if value not  u_long32
# -4   if unknown attribute
# 0    if ok
#
# name      template
# oticket   0
# fshare    0
# acl       NONE
# xacl      NONE
#
#  FUNCTION
#     Calls qconf -M(m)prj to modify project
#
#  INPUTS
#     project 	   - project we are modifying
#     attribute    - attribute of project we are modifying
#     value        - value of attribute of project we are modifying
#     {fast_add 1} - 0: modify the attribute using qconf -mckpt,
#                  - 1: modify the attribute using qconf -Mckpt, faster
#     {on_host ""} - execute qconf on this host, default is master host
#     {as_user ""} - execute qconf as this user, default is $CHECK_USER
#     raise_error  - do add_proc_error in case of errors
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************
proc mod_project {project change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1} } {
   global ts_config

   global CHECK_ARCH CHECK_OUTPUT
   global CHECK_CORE_MASTER CHECK_USER

   upvar $change_array old_values

   # add queue from file?
   if { $fast_add } {
      get_prj $project old_prj
      foreach elem [array names old_values] {
         set old_prj($elem) "$old_values($elem)"
      }
      
      set tmpfile [dump_array_to_tmpfile old_prj]
      set result [start_sge_bin "qconf" "-Mprj $tmpfile" $on_host $as_user]

      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [mod_project_error $result $tmpfile $raise_error]
      }

   } else {

      set vi_commands [build_vi_command old_values]

      set MODIFIED [translate_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS "*" "*" "*" "*" ]
      set ALREADY_EXISTS [ translate_macro MSG_SGETEXT_ALREADYEXISTS_SS "*" "*"]
      set NOT_MODIFIED [translate_macro MSG_FILE_NOTCHANGED ]
      set NOTULONG [ translate_macro MSG_OBJECT_VALUENOTULONG_S "*" ]
      set UNKNOWN_ATTRIBUTE [ translate_macro MSG_UNKNOWNATTRIBUTENAME_S "*" ]

      set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mprj $project" $vi_commands $MODIFIED $ALREADY_EXISTS $NOTULONG $UNKNOWN_ATTRIBUTE $NOT_MODIFIED]

      if {$result == -1 } { 
         add_proc_error "mod_project" -1 "timeout error" $raise_error
      } elseif {$result == -2 } { 
         add_proc_error "mod_project" -1 "\"[set old_values(name)]\" already exists"  $raise_error
      } elseif {$result == -3 } { 
         add_proc_error "mod_project" -1 "value not  u_long32 value"  $raise_error
      } elseif {$result == -4 } { 
         add_proc_error "mod_project" -1 "unknown attribute "  $raise_error
      } elseif {$result != 0  } { 
         add_proc_error "mod_project" -1 "could not add project \"[set old_values(name)]\""  
      }

      set ret $result
   }
   return $ret
}

#****** sge_project/mod_project_error() ***************************************
#  NAME
#     mod_project_error() -- error handling for mod_project
#
#  SYNOPSIS
#     mod_project_error {result attribute value tmpfile raise_error }
#
#  FUNCTION
#     Does the error handling for mod_project.
#     Translates possible error messages of qconf -Mprj,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_project. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     attribute   - attribute  qconf is modifying
#     value       - value qconf is modifying
#     tmpfile     - temp file used by qconf
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_attr function:
#      -1: "wrong_attr" is not an attribute
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_project_error {result tmpfile raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 -2"
   set messages(-1) "error: [translate_macro MSG_UNKNOWNATTRIBUTENAME_S \"*\" ]"
   set messages(-2) [translate_macro MSG_OBJECT_VALUENOTULONG_S "*"]

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_attr" "qconf -Mprj $tmpfile " $result messages $raise_error]

   return $ret
}

proc test_project {} {
   global ts_config CHECK_OUTPUT

   # test adding projects
   set project(name) "test1"

   add_prj project 0 "" "" 0 ;# slow add
   del_prj "test1"

   add_prj project 1 "" "" 0 ;# fast add

   # test adding duplicates (must fail)
   add_prj project 0 "" "" 0 ;# slow add duplicate
   add_prj project 1 "" "" 0 ;# fast add duplicate

   del_prj "test1"

   # test other errors
   set project(name) "default"
   add_prj project 0 "" "" 0 ;# slow add incorrect prj (name is keyword)
   add_prj project 1 "" "" 0 ;# fast add incorrect prj (name is keyword)
}

