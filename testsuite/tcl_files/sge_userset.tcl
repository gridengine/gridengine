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

#                                                             max. column:     |
#****** sge_userset/add_userset() ******
#
#  NAME
#     add_userset -- add a userset with qconf -Au
#
#  SYNOPSIS
#     add_userset { change_array }
#
#  FUNCTION
#     ???
#
#  INPUTS
#     change_array - Array with the values for the userset
#               Values:
#                  name    name of the userset (required)
#                  type    ACL, DEPT
#                  fshare  functional shares
#                  oticket overwrite tickets
#                  entries user list
#
#  RESULT
#     0    userset added
#     else error
#
#  EXAMPLE
#     set  userset_conf(name)    "dep0"
#     set  userset_conf(type)    "DEPT"
#     set  userset_conf(oticket) "1000"
#     set  userset_conf(entries) "codtest1"
#
#     add_userset userset_conf
#
#  NOTES
#
#  BUGS
#     ???
#
#  SEE ALSO
#     tcl_files/sge_procedures/add_access_list
#     tcl_files/sge_procedures/del_access_list
#
#*******************************
proc add_userset { change_array } {
   global ts_config
   global CHECK_ARCH
   global CHECK_CORE_MASTER CHECK_USER CHECK_HOST CHECK_USER CHECK_OUTPUT
   upvar $change_array chgar
   set values [array names chgar]
  
   if { [ string compare $ts_config(product_type) "sge" ] == 0 } {
     add_proc_error "add_userset" -1 "not possible for sge systems"
     return -3
   }
   set ADDED [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ADDEDTOLIST_SSSS] $CHECK_USER "*" "*" "*"]
   set ALREADY_EXISTS [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SGETEXT_ALREADYEXISTS_SS] "*" "*"]
  
 set f_name [get_tmp_file_name]
   set fd [open $f_name "w"]
   set org_struct(name)    "template"
   set org_struct(type)    "ACL DEPT"
   set org_struct(oticket) "0"
   set org_struct(fshare)  "0"
   set org_struct(entries) "NONE"
   foreach elem $values {
     set org_struct($elem) $chgar($elem)
   }
   set ogr_struct_names [array names org_struct]
   foreach elem  $ogr_struct_names {
     puts $CHECK_OUTPUT "$elem $org_struct($elem)"
     puts $fd "$elem $org_struct($elem)"
   }
   close $fd
   puts $CHECK_OUTPUT "using file $f_name"
   set result [start_remote_prog $CHECK_HOST $CHECK_USER "qconf" "-Au $f_name"]
   if { $prg_exit_state != 0 } {
     add_proc_error "add_userset" -1 "error running qconf -Au"
   }
   set result [string trim $result]
   puts $CHECK_OUTPUT "\"$result\""
   #     puts $CHECK_OUTPUT "\"$ADDED\""
   if { [ string match "*$ADDED" $result] } {
     return 0
   }
   if { [ string match "*$ALREADY_EXISTS" $result] } {
     add_proc_error "add_user" -1 "\"[set chgar(name)]\" already exists"
     return -2
   }
   add_proc_error "add_user" -1 "\"error adding [set chgar(name)]\""
   return -100
}

