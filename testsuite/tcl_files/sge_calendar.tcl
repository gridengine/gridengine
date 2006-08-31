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
#     get_calendar { calendar {output_var result} {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -scal $calendar to retrieve calendar 
#
#  INPUTS
#     output_var      - result will be placed here
#     calendar        - value of calendar we wish to see; 
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
proc get_calendar {calendar {output_var result} {on_host ""} {as_user ""} {raise_error 1}} {

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
#     get_calender_list { {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -scall to retrieve all calendars 
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
proc get_calender_list {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_calender_list" "-scall" out $on_host $as_user $raise_error]

}

