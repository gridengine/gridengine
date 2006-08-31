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

#****** sge_calendar/get_calendar() *******************************************
#  NAME
#     get_calendar() -- get calendar $calendar
#
#  SYNOPSIS
#     get_calendar { calendar  {output_var result} {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -scal $calendar to retrieve calendar 
#
#  INPUTS
#     calendar        - value of calendar we wish to see; 
#     {output_var result} - result will be placed here
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
proc get_calendar {calendar {output_var result}  {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-scal $calendar" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [get_calendar_error $result $calendar $raise_error]
   }

   return $ret

}

#****** sge_calendar/get_calendar_error() ***************************************
#  NAME
#     get_calendar_error() -- error handling for get_calendar
#
#  SYNOPSIS
#     get_calendar_error { result calendar {arg "wrong_calendar"} raise_error }
#
#  FUNCTION
#     Does the error handling for get_calendar.
#     Translates possible error messages of qconf -scal,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     get_calendar. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     calendar    - calendar for which qconf -scal has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_calendar function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc get_calendar_error {result calendar raise_error} {
 
   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_calendar_error_vdep messages $calendar

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_calendar" "qconf -scal $calendar" $result messages $raise_error]

   return $ret
}

#****** sge_calendar/get_calender_list() *****************************************
#  NAME
#     get_calender_list() -- get a list of all calendars
#
#  SYNOPSIS
#     get_calender_list {  {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -scall to retrieve all calendars 
#
#  INPUTS
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
proc get_calender_list { {on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_calender_list" "-scall" out $on_host $as_user $raise_error]

}


#                                                             max. column:     |
#****** sge_calendar/add_calendar() ******
# 
#  NAME
#     add_calendar -- add new calendar definition object
#
#  SYNOPSIS
#     add_calendar { change_array {fast_add 1} {on_host ""} {as_user ""} } 
#
#  FUNCTION
#     This procedure will add/define a new calendar definition object
#
#  INPUTS
#     {fast_add 1} - if not 0 the add_calendar procedure will use a file for
#                    adding a calendar
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#
#  RESULT
#     -1   timeout error
#     -2   calendar already exists
#      0   ok
#
#  EXAMPLE
#     set new_cal(calendar_name)  "qconf_calendar"
#     set new_cal(year)           "NONE"
#     set new_cal(week)           "mon-sun=0-24=suspended" 
#
#  NOTES
#     The array should look like this:
#
#     set change_array(calendar_name) "mycalendar"
#     set change_array(year) 	        "NONE"
#     set change-array(week)          "mon-sun=0-24=suspended"
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#
#     attribute(calendar_name) "test"
#     attribute(year)          "NONE"
#     attribute(week)          "NONE"
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************
proc add_calendar {change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
  global ts_config CHECK_TESTSUITE_ROOT
  global env CHECK_ARCH open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

  upvar $change_array chgar

  set values [array names chgar]

  set default_array(calendar_name)    "template"
  set default_array(year)      "NONE"
  set default_array(week)      "NONE"

  foreach elem $values {
     puts $CHECK_OUTPUT "--> setting \"$elem\" to \"$chgar($elem)\""
     set default_array($elem) "$chgar($elem)"
  }

  # Set calendar to chgar(calendar_name)
  set calendar $chgar(calendar_name)

  if { $fast_add != 0 } {
     # add calendar from file!

     set tmpfile [get_tmp_file_name]
     set file [open $tmpfile "w"]
     set values [array names default_array]
     foreach elem $values {
        set value [set default_array($elem)]
        puts $file "$elem                   $value"
     }
     close $file

      set ret 0
      set result [start_sge_bin "qconf" "-Acal $tmpfile" $on_host $as_user]

      # parse output or raise error
      if {$prg_exit_state == 0} {
         set ret 0
      } else {
         set ret [add_calender_error $prg_exit_state $tmpfile $calendar $raise_error]
      }
   } else {

      puts $CHECK_OUTPUT "adding calendar $calendar"

      set vi_commands [build_vi_command chgar]
      set ADDED [translate_macro MSG_SGETEXT_ADDEDTOLIST_SSSS "*" "*" "*" "*"]
      set ALREADY_EXISTS [translate_macro MSG_SGETEXT_ALREADYEXISTS_SS "*" "*"]

     # Now add using vi
     set ret [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-acal $calendar" $vi_commands  $ADDED $ALREADY_EXISTS  ]
     if { $ret == -1 } { add_proc_error "add_calendar" -1 "timeout error" }
     if { $ret == -2 } { add_proc_error "add_calendar" -1 "\"[set $calendar]\" already exists" }
     if { $ret != 0  } { add_proc_error "add_calendar" -1 "could not add calendar \"[set $calendar]\"" }

  }
  return $ret

}

#****** sge_calendar/add_calender_error() ***************************************
#  NAME
#     add_calender_error() -- error handling for add_calendar
#
#  SYNOPSIS
#     add_calender_error { result tmpfile calendar raise_error }
#
#  FUNCTION
#     Does the error handling for add_calendar.
#     Translates possible error messages of qconf -Acal,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     add_calendar. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     tmpfile     - temp file with calendar info
#     calendar    - calendar we are adding
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_calendar function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc add_calender_error {result tmpfile calendar raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_calendar_error_vdep messages $calendar

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "add_calendar" "qconf -Acal $tmpfile" $result messages $raise_error]

   return $ret
}

#                                                             max. column:     |
#****** sge_fastadd/mod_calendar() ******
#
#  NAME
#     mod_calendar -- modify eixsting calendar 
#
#  SYNOPSIS
#     mod_calendar {change_array {fast_add 1} {on_host ""} {as_user ""} }
#
#  FUNCTION
#     This procedure will modify an existing calendar 
#
#  INPUTS
#     change_array - name of an array variable that will be set by mod_calendar
#     {fast_add 1} - if not 0 the add_calendar procedure will use a file for
#                    adding a calendar
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#
#  RESULT
#     -1   timeout error
#     -2   calendar already exists
#      0   ok
#
#  EXAMPLE
#     set new_cal(calendar_name)  "qconf_calendar"
#     set new_cal(year)           "NONE"
#     set new_cal(week)           "mon-sun=0-24=suspended"
#
#  NOTES
#     The array should look like this:
#
#     set change_array(calendar_name) "mycalendar"
#     set change_array(year)            "NONE"
#     set change-array(week)          "mon-sun=0-24=suspended"
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#
#     attribute(calendar_name) "test"
#     attribute(year)          "NONE"
#     attribute(week)          "NONE"
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************
proc mod_calendar {change_array {fast_add 1} {on_host ""} {as_user ""} {raise_error 1}} {
  global ts_config CHECK_TESTSUITE_ROOT
  global env CHECK_ARCH open_spawn_buffer
  global CHECK_CORE_MASTER CHECK_USER CHECK_OUTPUT

  upvar $change_array chgar

  set values [array names chgar]

  set default_array(calendar_name)    "template"
  set default_array(year)      "NONE"
  set default_array(week)      "NONE"

  foreach elem $values {
     puts $CHECK_OUTPUT "--> setting \"$elem\" to \"$chgar($elem)\""
     set default_array($elem) $chgar($elem)
  }

  # Set calendar to chgar(calendar_name)
  set calendar $chgar(calendar_name)

 if { $fast_add != 0 } {
     # add calendar from file!

     set tmpfile  [get_tmp_file_name]
     set file [open $tmpfile "w"]
     set values [array names default_array]
     foreach elem $values {
        set value [set default_array($elem)]
        puts $file "$elem                   $value"
     }
     close $file

     set ret 0
     set result [start_sge_bin "qconf" "-Mcal $tmpfile" $on_host $as_user]

     # parse output or raise error
     if {$prg_exit_state == 0} {
        set ret 0 
     } else {
     set ret [mod_calender_error $prg_exit_state $tmpfile $calendar $raise_error]
     }

   } else {

      puts $CHECK_OUTPUT "modifying calendar $calendar"

      set vi_commands [build_vi_command chgar]

      set MODIFIED [translate_macro MSG_SGETEXT_MODIFIEDINLIST_SSSS "*" "*" "*" "*"]
      set ALREADY_EXISTS [translate_macro  MSG_SGETEXT_ALREADYEXISTS_SS "*" "*" ]

     # Now add using vi
     set ret [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-mcal $calendar" $vi_commands $MODIFIED $ALREADY_EXISTS ]
     if { $ret == -1 } { add_proc_error "add_calendar" -1 "timeout error" }
     if { $ret == -2 } { add_proc_error "add_calendar" -1 "\"[set $calendar]\" already exists" }
     if { $ret != 0  } { add_proc_error "add_calendar" -1 "could not add calendar \"[set $calendar]\"" }

  }
  return $ret

}
#****** sge_calendar/mod_calender_error() ***************************************
#  NAME
#     mod_calender_error() -- error handling for mod_calendar
#
#  SYNOPSIS
#     mod_calender_error { result tmpfile calendar raise_error }
#
#  FUNCTION
#     Does the error handling for add_calendar.
#     Translates possible error messages of qconf -Acal,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     add_calendar. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     tmpfile     - temp file with calendar info
#     calendar    - calendar we are adding
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_calendar function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_calender_error {result tmpfile calendar raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_calendar_error_vdep messages $calendar

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_calendar" "qconf -Mcal $tmpfile" $result messages $raise_error]

   return $ret
}


#****** sge_calendar/del_calendar() *******************************************
#  NAME
#     del_calendar() -- delete calendar $calendar
#
#  SYNOPSIS
#     del_calendar { calendar {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -dcal $calendar to delete calendar
#
#  INPUTS
#     calendar        - value of calendar we wish to delete;
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
proc del_calendar {calendar {on_host ""} {as_user ""} {raise_error 1}} {

   set ret 0
   set result [start_sge_bin "qconf" "-dcal $calendar" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      set ret 0
   } else {
      set ret [del_calendar_error $prg_exit_state $calendar $raise_error]
   }

   return $ret

}

#****** sge_calendar/del_calendar_error() ***************************************
#  NAME
#     del_calendar_error() -- error handling for del_calendar
#
#  SYNOPSIS
#     del_calendar_error { result calendar raise_error }
#
#  FUNCTION
#     Does the error handling for del_calendar.
#     Translates possible error messages of qconf -dcal,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     del_calendar. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     calendar    - calendar we are adding
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for get_calendar function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc del_calendar_error {result calendar raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_calendar_error_vdep messages $calendar

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "del_calendar" "qconf -dcal $calendar" $result messages $raise_error]

   return $ret
}

