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


#****** report/report_create() **************************************************
#  NAME
#    report_create() -- Create a report object
#
#  SYNOPSIS
#    report_create { name } 
#
#  FUNCTION
#     Creates a report object
#
#  INPUTS
#    name           -- name of the report object
#    a_report_array -- the report object
#    send_email     -- should a email send at the end of the report
#    write_html     -- should the html version of the report be written
#
#  RESULT
#     the id of the report object
#
#  EXAMPLE
#
#   array set report {}
#   report_create "Test report" report
#
#   report_add_message report "a foo message"
#
#  NOTES
#
#  BUGS
#
#  SEE ALSO
#     report/report_finish
#*******************************************************************************
proc report_create { name a_report_array { send_email 1 } { write_html 1 } } {   
   upvar $a_report_array report_array
   set report_array(name) $name
   set report_array(start) [exec date]
   set report_array(task_count) 0
   set report_array(messages) {}
   
   set report_array(handler) {}
   set report_array(task_progress_handler) {}
   
   if { $send_email == 1 } {
      lappend report_array(handler) report_send_mail
   }
   if { $write_html == 1 } {
      lappend report_array(handler) report_write_html
      lappend report_array(task_progress_handler) report_write_html
   }
}

#****** report_procedures/report_add_message() **************************************************
#  NAME
#    report_add_message() -- add a message to the report
#
#  SYNOPSIS
#    report_add_message { report message } 
#
#  FUNCTION
#     adds a message to the report. 
#
#  INPUTS
#    a_report  -- the report object
#    message   -- the message
#
#  RESULT
#
#  EXAMPLE
#
#   array set report {}
#   report_create "Test report" report
#
#   report_add_message report "a foo message"
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc report_add_message { a_report message } {
   global CHECK_OUTPUT
   upvar $a_report report_array
   lappend report_array(messages)  $message
   puts $CHECK_OUTPUT $message
}

#****** report_procedures/report_clear_messages() **************************************************
#  NAME
#    report_clear_messages() -- clear all messages of a report
#
#  SYNOPSIS
#    report_clear_messages { report } 
#
#  FUNCTION
#
#   The method removes all messages of a report
#
#  INPUTS
#    report -- the report object
# 
#  RESULT
#
#  EXAMPLE
#
#   array set report {}
#   report_create "Test report report
#
#   report_add_message report "a foo message"
# 
#   report_write_html report
#
#   report_clear_messages report
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc report_clear_messages { report } {
   upvar $report report_array
   set report_array(messages) {}
}


#****** report_procedures/report_create_task() **************************************************
#  NAME
#    report_create_task() -- create a task of a report
#
#  SYNOPSIS
#    report_create_task { report name host} 
#
#  FUNCTION
#    Creates a task for a report. All tasks of a report will be shown in
#    a table. 
#
#  INPUTS
#    report    --  the report object
#    name      --  Name of the tasks
#    host      --  Host where the task is running
#
#  RESULT
#
#  EXAMPLE
#
#  array set report {}
#  report_create "Test Report" report
#  ...
#  set task_nr [report_create_task report "test_task" "foo.bar"]
#  ...
#  set result ....
#  report_finish_task report $task_nr $result
#
#  NOTES
#
#  BUGS
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc report_create_task { report name host } {
   global CHECK_HTML_DIRECTORY CHECK_PROTOCOL_DIR

   upvar $report report_array
   set task_nr $report_array(task_count)
   incr report_array(task_count) 1
   
   set report_array(task_$task_nr,name)   $name
   set report_array(task_$task_nr,host)   $host
   set report_array(task_$task_nr,status) started
   set report_array(task_$task_nr,date)   [exec date]
   
   set relative_filename "${host}_${name}.txt"
   
   if { $CHECK_HTML_DIRECTORY != "" } {
      set myfilename "$CHECK_HTML_DIRECTORY/$relative_filename"
   } else {
      set myfilename "$CHECK_PROTOCOL_DIR/$relative_filename"
   }
   catch { file delete $myfilename }
   set report_array(task_$task_nr,filename) $myfilename
   set report_array(task_$task_nr,relative_filename) $relative_filename
   set report_array(task_$task_nr,file) [open $myfilename w]


   foreach handler $report_array(task_progress_handler) { 
      $handler report_array
   }
   return $task_nr
}


#****** report_procedures/report_task_add_message() ****************************
#  NAME
#    report_task_add_message() -- add a message to a task
#
#  SYNOPSIS
#    report_task_add_message { report task_nr message  } 
#
#  FUNCTION
#     
#     Add a message to a task
#     The message is written into the task file
#     and to CHECK_OUTPUT
#
#  INPUTS
#    report    --  the report object
#    task_nr   --  the number of the task
#    message   --  the message
#
#  RESULT
#
#  EXAMPLE
#  set task_nr [report_create_task report "test_task" "foo.bar"
#  ...
#  set result ....
#  report_task_add_message report $task_nr "foo_bar returned $result"
#
#  report_finish_task report $task_nr $result
#
#  NOTES
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc report_task_add_message { report task_nr message } {
   global CHECK_OUTPUT
   
   upvar $report report_array
   
   puts $report_array(task_$task_nr,file) $message
   flush $report_array(task_$task_nr,file)
   puts $CHECK_OUTPUT $message
}

#****** report_procedures/report_finish_task() **************************************************
#  NAME
#    report_finish_task() -- Mark a report task as finished
#
#  SYNOPSIS
#    report_finish_task { report task_nr result } 
#
#  FUNCTION
#     Mark a report task as finished.
#     The report task file will be flushed and closed
#     The result of the task is set

#  INPUTS
#    report    -- the report object 
#    task_nr   -- the task_nr
#    result    -- the result of the task
#
#  RESULT
#
#
#  EXAMPLE
#  set task_nr [report_create_task report "test_task" "foo.bar"
#  ...
#  set result ....
#  report_task_add_message report $task_nr "foo_bar returned $result"
#
#  report_finish_task report $task_nr $result
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc report_finish_task { report task_nr result } {
   
   upvar $report report_array

   if { $result == 0 } {
      set result "success"
   } else {
      set result "error"
   }
   set report_array(task_$task_nr,status) $result
   flush $report_array(task_$task_nr,file)
   close $report_array(task_$task_nr,file)
   set report_array(task_$task_nr,file) "--"

   foreach handler $report_array(task_progress_handler) { 
      $handler report_array
   }
}

#****** report_procedures/report_finish() **************************************************
#  NAME
#    report_finish() -- Mark a report as finished
#
#  SYNOPSIS
#    report_finish { report result } 
#
#  FUNCTION
#     Mark a report as finished
#     A email with the content of the report is send
#     A html file with the content of the report is written
#
#  INPUTS
#    report    --  the report object
#    result    --  the result of the report (numeric error code)
#
#  RESULT
#
#  EXAMPLE
#
#   array set report {}
#   report_create "Test report" report
#   ...
#   report_finish report 0
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc report_finish { report result } {
   
   upvar $report report_array

   if { $result == 0 } {
      set result "success"
   } else {
      set result "error"
   }
   
   set report_array(result) $result
   set report_array(end)    [exec date]
 
   foreach handler $report_array(handler) {
      $handler report_array
   }
}

#****** report_procedures/report_send_mail() ***********************************
#  NAME
#     report_send_mail() -- writes a report email 
#
#  SYNOPSIS
#     report_send_mail { report } 
#
#  FUNCTION
#     writes an report email 
#
#  INPUTS
#     report - the report object 
#
#*******************************************************************************
proc report_send_mail { report } {
   upvar $report report_array
   
   set mail_subject "testsuite - $report_array(name) -- "
   set mail_body    "testsuite - $report_array(name)\n"
   append mail_body "------------------------------------------\n\n"
   append mail_body " started: $report_array(start)\n"
   if { [info exists report_array(result)] } {
      append mail_subject $report_array(result)
      append mail_body "finished: $report_array(end)\n"
      append mail_body "  result: $report_array(result)\n"
   } else {
      append mail_subject "yet not finished"
      
   }
   append mail_body "------------------------------------------\n"
   
   if { [info exists report_array(task_0,name)] } {
      append mail_body "\nTasks:\n"
      
      set line [format "  %26s %12s %8s %s" "Name" "Host" "Status" "Details"]
      append mail_body "$line\n\n"
      
      for { set task_nr 0 } { [info exists report_array(task_$task_nr,name)] } { incr task_nr 1 } {
         
         set line [format "  %26s %12s %8s %s" $report_array(task_$task_nr,name) \
                                                $report_array(task_$task_nr,host) \
                                                $report_array(task_$task_nr,status) \
                                                "file://$report_array(task_$task_nr,filename)" ]
         append mail_body "$line\n"
      }
   }
   append mail_body "\n------------------------------------------\n"
   
   foreach message $report_array(messages) {
      append mail_body "$message\n"
   }
   append mail_body "------------------------------------------\n"
   
   mail_report $mail_subject $mail_body
}

#****** report_procedures/report_write_html() **********************************
#  NAME
#     report_write_html() -- report handler which write a html report
#
#  SYNOPSIS
#     report_write_html { report } 
#
#  FUNCTION
#     This report handler writes a html report into  CHECK_HTML_DIRECTORY
#
#  INPUTS
#     report - the report object
#
#*******************************************************************************
proc report_write_html { report } {

   global CHECK_HTML_DIRECTORY
   
   
   if { $CHECK_HTML_DIRECTORY == "" } {
      return
   }
   upvar $report report_array
   
   set html_body   [ create_html_text "started:   $report_array(start)" 1 ]
   
   if { [info exists report_array(result)] } {
      append html_body [ create_html_text "finished: $report_array(end)" 1 ]
      append html_body [ create_html_text "result: $report_array(result)" 1 ]
   } else {
      append html_body [ create_html_text "yet not finished" 1 ]
   }
   
   if { [info exists report_array(task_0,name)] } {
      append html_body [ create_html_text "<H1>Tasks:</H1>" 1 ]
      
      set html_table(1,BGCOLOR) "#3366FF"
      set html_table(1,FNCOLOR) "#66FFFF"
   
      set html_table(COLS) 5
      set html_table(1,1) "Name"
      set html_table(1,2) "Host"
      set html_table(1,3) "Arch"
      set html_table(1,4) "State"
      set html_table(1,5) "Details"
      
      set row_count 1
      for { set task_nr 0 } { [info exists report_array(task_$task_nr,name)] } { incr task_nr 1 } {
         incr row_count 1
         
         if { $report_array(task_$task_nr,status) == "error" } {
            set html_table($row_count,BGCOLOR) "#CC0000"
            set html_table($row_count,FNCOLOR) "#FFFFFF"
         } else {
            set html_table($row_count,BGCOLOR) "#009900"
            set html_table($row_count,FNCOLOR) "#FFFFFF"
         }
         set html_table($row_count,1) $report_array(task_$task_nr,name)
         set html_table($row_count,2) $report_array(task_$task_nr,host)
         set html_table($row_count,3) [resolve_arch $report_array(task_$task_nr,host)]
         set html_table($row_count,4) $report_array(task_$task_nr,status)
         set html_table($row_count,5) [ create_html_link $report_array(task_$task_nr,relative_filename) "./$report_array(task_$task_nr,relative_filename)"]      
      }
      set html_table(ROWS) $row_count

      append html_body [ create_html_table html_table ]
   }  else {
      append html_body [ create_html_text "No Tasks available" 1 ]
   }
   
   foreach message $report_array(messages) {
      append html_body [ create_html_text "$message" 0 ]
   }
   
   update_compile_html_output $html_body
   
}
