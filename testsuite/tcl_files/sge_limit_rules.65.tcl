#!/vol2/TCL_TK/glinux/bin/expect
# expect script 
# test SGE/SGEEE System
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

proc get_lirs {output_var {lirs ""} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config
   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-slrs $lirs" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_lirs_record result out
   } else {
      set ret [get_lirs_error $result $lirs $raise_error]
   }

   return $ret
}

proc get_lirs_list {output_var {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_lirs_list" "-slrsl" out $on_host $as_user $rais_error]
}

proc get_lirs_error {result lirs raise_error } {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_NOLIRSFOUND]

   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_lirs" "qconf -slrs $lirs" $result messages $raise_error]

   return $ret
}

proc add_lirs {change_array {fast_add 1} {on_host ""} {as_user ""}} {
   global ts_config CHECK_OUTPUT
   global env CHECK_ARCH
   global CHECK_CORE_MASTER

   upvar $change_array chgar

   # Modify lirs from file?
   if { $fast_add } {
      set tmpfile [dump_lirs_array_to_tmpfile chgar]
      set result [start_sge_bin "qconf" "-Alrs $tmpfile" $on_host $as_user ]

      # TODO: error handling

   } else {
   # Use vi
   }
  return $result
}

proc mod_lirs {change_array {name ""} {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT
   global env CHECK_ARCH
   global CHECK_CORE_MASTER
   
   upvar $change_array chgar

   set new_values [array names chgar]

   get_lirs old_values

   foreach elem $new_values {
      set old_values($elem) "$chgar($elem)"
   }

   # Modify lirs from file?
   if { $fast_add } {
      set tmpfile [dump_lirs_array_to_tmpfile old_values]
      puts $CHECK_OUTPUT "tmpfile: $tmpfile"
      set result [start_sge_bin "qconf" "-Mlrs $tmpfile" $on_host $as_user]

      # TODO: error handling

   } else {
      # Use vi
   }
}
