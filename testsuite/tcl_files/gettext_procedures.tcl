#!/vol2/TCL_TK/glinux/bin/tclsh
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

global macro_messages_list

#                                                             max. column:     |
#****** gettext_procedures/test_file() ******
# 
#  NAME
#     test_file -- test procedure 
#
#  SYNOPSIS
#     test_file { me two } 
#
#  FUNCTION
#     this function is just for test the correct function call 
#
#  INPUTS
#     me  - first output parameter 
#     two - second output parameter 
#
#  RESULT
#     output to stdout: 
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
proc test_file { me two} {
  global CHECK_OUTPUT
  puts $CHECK_OUTPUT "printing \"$me\" \"$two\". host is [exec hostname]" 
  return "test ok"
}

proc get_macro_messages_file_name { } {
  global CHECK_PROTOCOL_DIR CHECK_SOURCE_CVS_RELEASE CHECK_OUTPUT
  
  puts $CHECK_OUTPUT "checking messages file ..."
  if { [ file isdirectory $CHECK_PROTOCOL_DIR] != 1 } {
     file mkdir $CHECK_PROTOCOL_DIR
     puts $CHECK_OUTPUT "creating directory: $CHECK_PROTOCOL_DIR"
  }
  set filename $CHECK_PROTOCOL_DIR/source_code_macros_${CHECK_SOURCE_CVS_RELEASE}.dump
  return $filename
}

proc search_for_macros_in_c_source_code_files { file_list search_macro_list} {
   global CHECK_OUTPUT macro_messages_list

   if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
   }

   set search_list $search_macro_list

   puts $CHECK_OUTPUT "macro count: $macro_messages_list(0)"
   foreach file $file_list {
      puts $CHECK_OUTPUT "file: $file"
      puts $CHECK_OUTPUT "macros in list: [llength $search_list]"
      flush $CHECK_OUTPUT 
      set file_p [ open $file r ]
 
      set file_content ""
      while { [gets $file_p line] >= 0 } {
         append file_content $line
      }
      close $file_p
      set found ""
      foreach macro $search_list {
         if { [string first $macro $file_content] >= 0 } {
            lappend found $macro
         }
      }
      foreach macro $found {
         set index [lsearch -exact $search_list $macro]
         if { $index >= 0 } {
            set search_list [lreplace $search_list $index $index]
         }
      }
   }
   return $search_list 
}

#****** gettext_procedures/check_c_source_code_files_for_macros() **************
#  NAME
#     check_c_source_code_files_for_macros() -- check if macros are used in code
#
#  SYNOPSIS
#     check_c_source_code_files_for_macros { } 
#
#  FUNCTION
#     This procedure tries to find all sge macros in the source code *.c files.
#     If not all macros are found, an error message is generated.
#
#  NOTES
#     This procedure is called from update_macro_messages_list() after re-
#     parsing the source code for macros.
#
#  SEE ALSO
#     gettext_procedures/update_macro_messages_list()
#*******************************************************************************
proc check_c_source_code_files_for_macros {} {
   global CHECK_OUTPUT CHECK_SOURCE_DIR macro_messages_list check_name

   puts $CHECK_OUTPUT "check_name: $check_name"

   if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
   }


   set c_files ""
   set second_run_files ""

   set dirs [get_all_subdirectories $CHECK_SOURCE_DIR ]
   foreach dir $dirs {
      set files [get_file_names $CHECK_SOURCE_DIR/$dir "*.c"]
      foreach file $files { 
         if { [string first "qmon" $file] >= 0 } {
            lappend second_run_files $CHECK_SOURCE_DIR/$dir/$file
            continue
         }
         if { [string first "3rdparty" $dir] >= 0 } {
            lappend second_run_files $CHECK_SOURCE_DIR/$dir/$file
            continue
         }
         lappend c_files $CHECK_SOURCE_DIR/$dir/$file
      }
      set files [get_file_names $CHECK_SOURCE_DIR/$dir "*.h"]
      foreach file $files { 
         if { [string match -nocase msg_*.h $file] } {
            continue
         }
         lappend second_run_files $CHECK_SOURCE_DIR/$dir/$file
      }
   }

   set search_list ""
   for {set i 1} {$i <= $macro_messages_list(0) } {incr i 1} {
      lappend search_list $macro_messages_list($i,macro)
   }

   set search_list [search_for_macros_in_c_source_code_files $c_files $search_list ]
   set search_list [search_for_macros_in_c_source_code_files $second_run_files $search_list ]

   
   # remove SGE_INFOTEXT_TESTSTRING_S_L10N from searchlist
   set index [lsearch -exact $search_list "SGE_INFOTEXT_TESTSTRING_S_L10N"]
   if { $index >= 0 } {
      set search_list [lreplace $search_list $index $index]
   }

   set answer ""
   foreach macro $search_list {
      append answer "   $macro\n"
   }
 
   if { [llength $search_list ] > 0 } {
      set full_answer ""
      append full_answer "following macros seems not to be used in source code:\n"
      append full_answer "$CHECK_SOURCE_DIR\n\n"
      append full_answer "---------------------------------------------------------------\n"
      append full_answer $answer
      append full_answer "---------------------------------------------------------------\n"

      add_proc_error "check_c_source_code_files_for_macros" -3 $full_answer
   }
#
# uncomment the following lines, if the unused macros should be removed from source code
# ======================================================================================  
# 
# --   foreach macro $search_list {
# --      set id [get_macro_id_from_name $macro]
# --      set index [get_internal_message_number_from_id $id]
# --   
# --      set file $macro_messages_list($index,file)
# --      set file_ext 1
# --      puts $CHECK_OUTPUT $macro_messages_list($index,macro)
# --      puts $CHECK_OUTPUT $file
# --      read_file $file file_dat
# --      set lines $file_dat(0)
# --      set changed 0
# --      for { set i 1 } { $i <= $lines } { incr i 1 } {
# --         if { [ string first $macro $file_dat($i) ] >= 1 && 
# --              [ string first "_MESSAGE" $file_dat($i) ] >= 1 } {
# --            puts $CHECK_OUTPUT $file_dat($i)
# --            set message_pos [ string first "_MESSAGE" $file_dat($i) ]
# --            set new_line "/* "
# --            incr message_pos -1
# --            append new_line [ string range $file_dat($i) 0 $message_pos]
# --            append new_line "_message"
# --            incr message_pos 9
# --            append new_line [ string range $file_dat($i) $message_pos end ]
# --            append new_line " __TS Removed automatically from testsuite!! TS__*/"
# --            puts $CHECK_OUTPUT $new_line
# --            set file_dat($i) $new_line
# --            set changed 1
# --         }
# --      }
# --      if { $changed == 1 } {
# --         while { 1 } {
# --         set catch_return [ catch { 
# --               file rename $file "$file.tmp${file_ext}" 
# --         } ]
# --            incr file_ext 1
# --            puts $CHECK_OUTPUT "catch: $catch_return"
# --            if { $catch_return == 0 } {
# --               break
# --            }
# --         }
# --         save_file $file file_dat
# --      }
# --   }
# --   puts $CHECK_OUTPUT "macros removed"
# --   wait_for_enter
}

#****** gettext_procedures/update_macro_messages_list() ************************
#  NAME
#     update_macro_messages_list() -- parse sge source code for sge macros
#
#  SYNOPSIS
#     update_macro_messages_list { } 
#
#  FUNCTION
#     This procedure reads all sge source code messages files (msg_*.h) in order
#     to get all macro strings and store it to the global variable
#     macro_messages_list.
#
#  NOTES
#     This procedure is called when the source code is updated ( procedure
#     compile_source() ) and when sge_macro() is called.
#
#  SEE ALSO
#     gettext_procedures/sge_macro()
#     check/compile_source()
#*******************************************************************************
proc update_macro_messages_list {} {
  global CHECK_OUTPUT CHECK_SOURCE_DIR macro_messages_list
  global CHECK_PROTOCOL_DIR CHECK_SOURCE_CVS_RELEASE 
  if { [info exists macro_messages_list]} {
     unset macro_messages_list
  }
  set filename [get_macro_messages_file_name]
  if { [ file isfile $filename ] } {
     puts $CHECK_OUTPUT "reading macro messages spool file:\n\"$filename\" ..."
     puts $CHECK_OUTPUT "delete this file if you want to parse the macros again!"
     puts $CHECK_OUTPUT "======================================================="
     read_array_from_file $filename "macro_messages_list" macro_messages_list 1 
     if { [ string compare $macro_messages_list(source_code_directory) $CHECK_SOURCE_DIR] != 0 } {
        puts $CHECK_OUTPUT "source code directory from macro spool file:"
        puts $CHECK_OUTPUT $macro_messages_list(source_code_directory)
        puts $CHECK_OUTPUT "actual source code directory:"
        puts $CHECK_OUTPUT $CHECK_SOURCE_DIR
        puts $CHECK_OUTPUT "the macro spool dir doesn't match to actual source code directory."
        puts $CHECK_OUTPUT "start parsing new source code directory ..."
        if { [info exists macro_messages_list]} {
           unset macro_messages_list
        }
     } else {
        return
     }
  }
  set error_text ""
  set msg_files ""
  set dirs [get_all_subdirectories $CHECK_SOURCE_DIR ]
  foreach dir $dirs {
     set files [get_file_names $CHECK_SOURCE_DIR/$dir "msg_*.h"]
     foreach file $files { 
       lappend msg_files $CHECK_SOURCE_DIR/$dir/$file
     }
  }
  set count 1
  puts $CHECK_OUTPUT "\nparsing source code for message macros ..."
  foreach file $msg_files {
     puts $CHECK_OUTPUT "file: $file"
     flush $CHECK_OUTPUT 
     set file_p [ open $file r ]
     while { [gets $file_p line] >= 0 } {
        if { [string first "_MESSAGE" $line] >= 0 } {
           set org_line $line
           set line [replace_string $line "SFQ" "\"\\\"%-.100s\\\"\""]
           set line [replace_string $line "SFN" "\"%-.100s\""]
           set line [replace_string $line "U32CFormat" "\"%ld\""]
           set line [replace_string $line "X32CFormat" "\"%lx\""]
           set line [replace_string $line "SN_UNLIMITED" "\"%s\""]
           set line [replace_string $line "_(SGE_INFOTEXT_TESTSTRING_S)" "_(\"Welcome, %s\\nhave a nice day!\\n\")"]
           set line [replace_string $line "_(SGE_INFOTEXT_UNDERLINE)" "_(\"-\""]

           set line [replace_string $line "\\\"" "___01815DUMMY___"]
           set got_error 0
           while { [ set old [replace_string $line "\"" "" 1]] != 2 } {
              set index [string first "\"" $line ]
              set new_line [ string range $line 0 $index ]
              incr index 1
              set help [ string range $line $index end ]
              set index [ string first "\"" $help ]
              incr index -1
              append new_line [string range $help 0 $index]
              incr index 2
              set help [ string range $help $index end ]
              set index [ string first "\"" $help ]
              set cut [ string range $help 0 [ expr ( $index - 1) ] ]
              set cut [ string trim $cut ]
              incr index 1
              append new_line [string range $help $index end]
              set line $new_line
              set new [replace_string $line "\"" "" 1]
              if { $old == $new } {
                 puts $CHECK_OUTPUT "error in update_macro_messages_list"
                 wait_for_enter
              }
              if { [string length $cut] > 0 } { 
                 set unexpected_specifier $cut
                 set got_error 1
              }

           }
           set line [replace_string $line "___01815DUMMY___" "\\\"" ]

           set message_id_start [string first "_MESSAGE" $line]
           set message_macro [ string range $line 0 [ expr ( $message_id_start - 1 ) ]]
           set help [ string range $line $message_id_start end]
           set message_id_start [string first "(" $help]
           set message_id_end   [string first "," $help]
           incr message_id_end -1
           incr message_id_start 1
           set message_id [ string range $help $message_id_start $message_id_end ]
           
           set message_string_start [string first "_(" $help]
           incr message_string_start 3
           set message_string [ string range $help $message_string_start end ]
           set message_string [replace_string $message_string "\\\"" "___01815DUMMY___" ]
           set message_string [replace_string $message_string "\"" "___01816DUMMY___" ]
           set message_string_end [ string first "___01816DUMMY___" $message_string ]
           incr message_string_end -1
           set message_string [ string range $message_string 0 $message_string_end] 
           set message_string [ replace_string $message_string "___01815DUMMY___" "\\\""]

           set index [ string first "#define" $message_macro]
           set message_macro [ string range $message_macro [ expr ( $index + 7 ) ] end]
           set message_macro [ string trim $message_macro]

           set macro_messages_list($count)        $line
           set macro_messages_list($count,id)     $message_id
           set macro_messages_list($count,macro)  $message_macro 
           set macro_messages_list($count,string) $message_string
           set macro_messages_list($count,file)   $file
           if { [ info exists macro_messages_list(0,$message_id)] != 0 } {
              append error_text "\n\n-----------MESSAGE-ID-NOT-UNIQUE----------\n"
              append error_text "message id $message_id is not unique\n"
              append error_text "$macro_messages_list(0,$message_id)\nand\n$line"
              puts $CHECK_OUTPUT "---\nmessage id $message_id is not unique"
              puts $CHECK_OUTPUT $macro_messages_list(0,$message_id)
              puts $CHECK_OUTPUT "and"
              puts $CHECK_OUTPUT $line
           }
           set macro_messages_list(0,$message_id) $line

           if { $got_error == 1 } {
               append error_text "\n\n-------UNEXPECTED-FORMAT-SPECIFIER-------\n"
               append error_text "error for message id $message_id in file \n$file:\n"
               append error_text "$org_line\nunexpected specifier: $unexpected_specifier"
               puts $CHECK_OUTPUT "---\nerror for message id $message_id in file \n$file:\n$org_line\nunexpected specifier: -->$unexpected_specifier<--"
           }
           incr count 1
        }
     }
     close $file_p
  }
  if { [string compare $error_text ""] != 0 } {
     add_proc_error "update_macro_messages_list" "-3" $error_text
  }
  incr count -1
  set macro_messages_list(0) $count
  puts $CHECK_OUTPUT "parsed $count messages."

  puts $CHECK_OUTPUT "saving macro file ..."
  
  set macro_messages_list(source_code_directory) $CHECK_SOURCE_DIR
  
  spool_array_to_file $filename "macro_messages_list" macro_messages_list
  check_c_source_code_files_for_macros
}

#****** gettext_procedures/get_macro_string_from_name() ************************
#  NAME
#     get_macro_string_from_name() -- get sge source code macro string from name
#
#  SYNOPSIS
#     get_macro_string_from_name { macro_name } 
#
#  FUNCTION
#     This procedure returns the string defined by the given macro
#
#  INPUTS
#     macro_name - sge macro name (etc.: MSG_XXXX_S )
#
#  RESULT
#     string
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc get_macro_string_from_name { macro_name } {
  global  CHECK_OUTPUT macro_messages_list

  if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
  }
  for {set i 1} {$i <= $macro_messages_list(0)} {incr i 1} {
     if { [string compare $macro_name $macro_messages_list($i,macro) ] == 0 } {
#        puts $CHECK_OUTPUT "found macro for message id $macro_messages_list($i,id)"
#        puts $CHECK_OUTPUT "macro number is $i"
        return $macro_messages_list($i,string);
     }
  }
  return -1
}

#****** gettext_procedures/sge_macro() *****************************************
#  NAME
#     sge_macro() -- return sge macro string
#
#  SYNOPSIS
#     sge_macro { macro_name } 
#
#  FUNCTION
#     This procedure returns the string defined by the macro. 
#
#  INPUTS
#     macro_name - sge source code macro
#
#  RESULT
#     string
#
#  EXAMPLE
#     set string [sge_macro MSG_OBJ_SHARETREE] 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc sge_macro { macro_name } {
   global CHECK_OUTPUT
 
   set value ""
   switch -exact $macro_name {
      "MSG_OBJ_QUEUE" { set value "" }
      "MSG_SGETEXT_ALREADYEXISTS_SS" { set value ""}
      "MSG_QCONF_NOXDEFINED_S" { set value "" }
      "MSG_QUEUE_XISNOTAQUEUENAME_S" { set value "" }
      "MSG_SGETEXT_MODIFIEDINLIST_SSSS" { set value "" }
      "MSG_SGETEXT_ADDEDTOLIST_SSSS" { set value "" }
      "MSG_SGETEXT_REMOVEDFROMLIST_SSSS" { set value "" }
      "MSG_SGETEXT_UNKNOWNQUEUE_SSSS"  { set value "" }
      "MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S" { set value "" }
      "MSG_SGETEXT_CONFIG_MODIFIEDINLIST_SSS" { set value "" }
      "MSG_SGETEXT_CONFIG_ADDEDTOLIST_SSS" { set value "" }
      "MSG_PARSE_EDITFAILED"       { set value "" }
      "MSG_MULTIPLY_MODIFIEDIN"   { set value "" }
      "MSG_MULTIPLY_ADDEDTO"   { set value "" }
      "MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION"  { set value "" }
      "MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S"  { set value "" }
      "MSG_SGETEXT_UNKNOWNUSERSET_SSSS" { set value "" }
      "MSG_SCHEDD_INFO_NOMESSAGE" { set value "" }
      "MSG_GDI_ADDTOACL_SS" { set value "" }
      "MSG_QUEUE_DISABLEQ_SSS" { set value "" }
      "MSG_QUEUE_ENABLEQ_SSS"  { set value "" }
      "MSG_PROJECT"      { set value "" }
      "MSG_SGETEXT_DOESNOTEXIST_S" { set value "" }
      "MSG_OBJ_SHARETREE" { set value "" }
      "MSG_SGETEXT_REMOVEDLIST_SSS" { set value "" }
      "MSG_JOB_SUBMITJOB_USS" { set value "" }
      "MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER" { set value "" }
      "MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U" { set value "" }
      "MSG_SGETEXT_MOD_JOBS_SU" { set value "" }
      "MSG_GDI_USAGE_USAGESTRING" { set value "" }
      "MSG_JOB_SUBMITJOBARRAY_UUUUSS" { set value "" }
      "MSG_JOB_MOD_JOBNETPREDECESSAMBIGUOUS_SUU" { set value "" }
      "MSG_ANSWER_UNKOWNOPTIONX_S"  { set value "" }
      "MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS" { set value "" }
      "MSG_STREE_USERTNOACCESS2PRJ_SS"  { set value "" }
      "MSG_JOB_NOPERMS_SS" { set value "" }
      "MSG_JOB_PRJNOSUBMITPERMS_S" { set value "" }
      "MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S" { set value "" }
      "MSG_SGETEXT_CANTRESOLVEHOST_S" { set value "" }
      "MSG_SGETEXT_UNKNOWN_RESOURCE_S" { set value "" }
      "MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE" { set value "" }
      "MSG_JOB_MORETASKSTHAN_U" { set value "" }
      "MSG_JOB_REGDELTASK_SUU" { set value "" }
      "MSG_JOB_REGDELJOB_SU" { set value "" }
      "MSG_JOB_DELETETASK_SUU" { set value "" }
      "MSG_JOB_DELETEJOB_SU" { set value "" }
      "MSG_QUEUE_SUSPENDQ_SSS" { set value "" }
      "MSG_QUEUE_UNSUSPENDQ_SSS" { set value "" }
      "MSG_JOB_SUSPENDTASK_SUU" { set value "" }
      "MSG_JOB_SUSPENDJOB_SU" { set value "" }
      "MSG_JOB_UNSUSPENDTASK_SUU" { set value "" }
      "MSG_JOB_UNSUSPENDJOB_SU" { set value "" }
      "MSG_SGETEXT_DOESNOTEXIST_SU" { set value "" }
      "MSG_SGETEXT_DOESNOTEXISTTASK_UU" { set value "" }
      "MSG_SGETEXT_DOESNOTEXISTTASKRANGE_UUUU" { set value "" }
      "MSG_QSH_INTERACTIVEJOBHASBEENSCHEDULED_D" { set value "" }
      "MSG_GDI_USING_SS" { set value "" }
      "MSG_QSH_REQUESTCANTBESCHEDULEDTRYLATER_S" { set value "" }
      "MSG_SGETEXT_NOSUBMITORADMINHOST_S" { set value "" }
      "MSG_SCHEDD_SCHEDULINGINFO" { set value "" }
      "MSG_SCHEDD_INFO_JOBHOLD_" { set value "" }
      "MSG_SGETEXT_MOD_JATASK_SUU" { set value "" }
      "MSG_SCHEDD_INFO_JOBINERROR_" { set value "" }
      "MSG_SCHEDD_INFO_EXECTIME_" { set value "" }
      "MSG_SCHEDD_INFO_JOBDEPEND_" { set value "" }
      "MSG_SCHEDD_INFO_MAX_AJ_INSTANCES_" { set value "" }
      "MSG_OBJ_USER"  { set value "" }
      "MSG_OBJ_USERSET" { set value "" }
      "MSG_SCHEDD_INFO_HASNOPERMISSION_SS" { set value "" }
      "MSG_SCHEDD_INFO_NOACCESSTOPE" { set value "" }
      "MSG_SCHEDD_INFO_NOACCESSTOPE_S" { set value "" }
      "MSG_JOB_NOTINANYQ_S" { set value "" }
      "MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S" { set value "" }
      "MSG_TREE_CHANGEDSHARETREE" { set value "" }
      "MSG_QCONF_CANTREADSHARETREEX_S" { set value "" }

      "DISTINST_HIT_RETURN_TO_CONTINUE" { set value "\nHit <RETURN> to continue >> " } 
      "DISTINST_NOT_COMPILED_IN_SECURE_MODE" { set value "\n>sge_qmaster< binary is not compiled with >-secure< option!\n" }
      "DISTINST_ENTER_HOSTS" { set value "Host(s): " }
      "DISTINST_MASTER_INSTALLATION_COMPLETE" { set value "\nYour Grid Engine qmaster installation is now completed" }
      "DISTINST_ENTER_A_RANGE" { set value "Please enter a range >> " }
      "DISTINST_PREVIOUS_SCREEN" { set value "Do you want to see previous screen about using Grid Engine again (y/n) \[n\] >> " }
      "DISTINST_FILE_FOR_HOSTLIST" { set value "Do you want to use a file which contains the list of hosts (y/n) \[n\] >> " }
      "DISTINST_FINISHED_ADDING_HOSTS" { set value "Finished adding hosts. Hit <RETURN> to continue >> " }
      "DISTINST_FILENAME_FOR_HOSTLIST" { set value "\nPlease enter the file name which contains the host list: " }
      "DISTINST_CREATE_NEW_CONFIGURATION" { set value "Do you want to create a new configuration (y/n) \[y\] >> " }
      "DISTINST_INSTALL_SCRIPT" { set value "\nWe can install the startup script that\nGrid Engine is started at machine boot (y/n) \[y\] >> " }
      "DISTINST_ANSWER_YES" { set value "y" }
      "DISTINST_ANSWER_NO" { set value "n" }
      "DISTINST_ENTER_DEFAULT_DOMAIN" { set value "\nPlease enter your default domain >> " }
      "DISTINST_CONFIGURE_DEFAULT_DOMAIN" { set value "Do you want to configure a default domain (y/n) \[y\] >> " }
      "DISTINST_PKGADD_QUESTION" { set value "Did you install this version with >pkgadd< or did you already\nverify and set the file permissions of your distribution (y/n) \[y\] >> " }
      "DISTINST_MESSAGES_LOGGING" { set value "Hit <RETURN> to see where Grid Engine logs messages >> " }
      "DISTINST_OTHER_SPOOL_DIR" { set value "Do you want to select another qmaster spool directory (y/n) \[n\] >> " }
      "DISTINST_OTHER_USER_ID_THAN_ROOT" { set value "Do you want to install Grid Engine\nunder an user id other than >root< (y/n) \[y\] >> " }
      "DISTINST_INSTALL_AS_ADMIN_USER" { set value "Do you want to install Grid Engine as admin user >%s< (y/n) \[y\] >> " }
      "DISTINST_ADMIN_USER_ACCOUNT" { set value "      admin user account = %s" }
      "DISTINST_USE_CONFIGURATION_PARAMS" { set value "\nDo you want to use these configuration parameters (y/n) \[y\] >> " }
      "DISTINST_INSTALL_GE_NOT_AS_ROOT" { set value "Do you want to install Grid Engine\nunder an user id other than >root< (y/n) \[y\] >> " }
      "DISTINST_IF_NOT_OK_STOP_INSTALLATION" { set value "Hit <RETURN> if this is ok or stop the installation with Ctrl-C >> " }
      "DISTINST_DNS_DOMAIN_QUESTION" { set value "Are all hosts of your cluster in a single DNS domain (y/n) \[y\] >> " }
      "DISTINST_ENTER_SPOOL_DIR_OR_HIT_RET" { set value "If you will install shadow master hosts or if you want to be able to start\n the qmaster daemon on other hosts (see the corresponding sectionin the\n Grid Engine Installation and Administration Manual for details) the account\n on the shadow master hosts also needs read/write access to this directory.\n\n Enter spool directory or hit <RETURN> to use default\n \[%s\] >> " }
      "DISTINST_USING_GID_RANGE_HIT_RETURN" { set value "\nUsing >%s< as gid range. Hit <RETURN> to continue >> " }
      "DISTINST_EXECD_INSTALL_COMPLETE" { set value "Your execution daemon installation is now completed." }
      "DISTINST_LOCAL_CONFIG_FOR_HOST" { set value "Local configuration for host >%s< created." }
      "DISTINST_CELL_NAME_FOR_QMASTER" { set value "\nPlease enter cell name which you used for the qmaster\ninstallation or press <RETURN> to use default cell >default< >> " }
      "DISTINST_ADD_DEFAULT_QUEUE" { set value "Do you want to add a default queue for this host (y/n) \[y\] >> " }
      "DISTINST_ALL_QUEUE_HOSTGROUP" { set value "Creating the default <all.q> queue and <allhosts> hostgroup" }
      "DISTINST_ADD_DEFAULT_QUEUE_INSTANCE" { set value "Do you want to add a default queue instance for this host (y/n) \[y\] >> " }

      "DISTINST_ENTER_DATABASE_SERVER" { set value "Enter database server (none for local spooling)\nor hit <RETURN> to use default \[%s\] >> " }
      "DISTINST_ENTER_DATABASE_DIRECTORY" { set value "Enter the database directory\nor hit <RETURN> to use default \[%s\] >> " }
      "DISTINST_DATABASE_DIR_NOT_ON_LOCAL_FS" { set value "The database directory >%s<\nis not on a local filesystem.\nPlease choose a local filesystem or configure the RPC Client/Server mechanism" }
      "DISTINST_STARTUP_RPC_SERVER" { set value "Please startup the rc script >%s< on the RPC server machine" }
      "DISTINST_DONT_KNOW_HOW_TO_TEST_FOR_LOCAL_FS" { set value "Don't know how to test for local filesystem. Exit." }

   }
   if { $value == "" } {
      set value [get_macro_string_from_name $macro_name]
#      puts $CHECK_OUTPUT "value for $macro_name is \n\"$value\""
   }
   if { $value == -1 } {
      set macro_messages_file [get_macro_messages_file_name]
      add_proc_error "sge_macro" -3 "could not find macro \"$macro_name\" in source code!!!\ndeleting macro messages file:\n$macro_messages_file"
      if { [ file isfile $macro_messages_file] } {
         file delete $macro_messages_file
      }
      update_macro_messages_list
   }

   return $value
}

proc get_macro_id_from_name { macro_name } {
  global  CHECK_OUTPUT macro_messages_list

  if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
  }
  for {set i 1} {$i <= $macro_messages_list(0)} {incr i 1} {
     if { [string compare $macro_name $macro_messages_list($i,macro) ] == 0 } {
#        puts $CHECK_OUTPUT "found macro for message id $macro_messages_list($i,id)"
        return $macro_messages_list($i,id);
     }
  }
  return -1
}


proc get_macro_string_from_id { id } {
  global  CHECK_OUTPUT macro_messages_list

  if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
  }
  for {set i 1} {$i <= $macro_messages_list(0)} {incr i 1} {
     if { $id == $macro_messages_list($i,id) } {
#        puts $CHECK_OUTPUT "found macro for message macro $macro_messages_list($i,macro)"
        return $macro_messages_list($i,string);
     }
  }
  return -1
}

proc get_internal_message_number_from_id { id } {
  global  CHECK_OUTPUT macro_messages_list

  if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
  }
  for {set i 1} {$i <= $macro_messages_list(0)} {incr i 1} {
     if { $id == $macro_messages_list($i,id) } {
#        puts $CHECK_OUTPUT "found macro for message macro $macro_messages_list($i,macro)"
        return $i
     }
  }
  return -1
}



proc translate_all_macros {} {
  global  CHECK_OUTPUT macro_messages_list CHECK_HOST
  global CHECK_SOURCE_DIR CHECK_HOST CHECK_USER
  if { [info exists macro_messages_list] == 0 } {
     update_macro_messages_list
  }
  set not_localized ""
  set max_mess $macro_messages_list(0)
  
#  set max_mess [get_internal_message_number_from_id 43262]
  set max_mess 2270 
  file delete /tmp/unused_macros.txt
  for {set i 2269} {$i <= $max_mess} {incr i 1} {
     puts $CHECK_OUTPUT "-------$i---------------"
     set format_string $macro_messages_list($i,string)

     set localized [translate $CHECK_HOST 0 0 1 $format_string]
     set localized [ replace_string $localized "\r" "__REP_1_DUMMY_"]
     set localized [ replace_string $localized "\t" "__REP_2_DUMMY_"]
     set localized [ replace_string $localized "\n" "__REP_3_DUMMY_"]
     set localized [ replace_string $localized "\0" "__REP_4_DUMMY_"]
     set localized [ replace_string $localized "\"" "__REP_5_DUMMY_"]
     set localized [ replace_string $localized "\\\'" "__REP_6_DUMMY_"]
     set localized [ replace_string $localized "\\" "__REP_7_DUMMY_"]
     

     
     set localized [ replace_string $localized "__REP_1_DUMMY_" ""]
     set localized [ replace_string $localized "__REP_2_DUMMY_" "\\t"]
     set localized [ replace_string $localized "__REP_3_DUMMY_" "\\n"]
     set localized [ replace_string $localized "__REP_4_DUMMY_" "\\0"]
     set localized [ replace_string $localized "__REP_5_DUMMY_" "\\\""]
     set localized [ replace_string $localized "__REP_6_DUMMY_" "\\\'"]
     set localized [ replace_string $localized "__REP_7_DUMMY_" "\\\\"]

 
     
     set localized [ string trim $localized]
     set format_string [ string trim $format_string ]
     puts $CHECK_OUTPUT ">$format_string<"
     puts $CHECK_OUTPUT ">$localized<"
     
     if { [string compare $format_string $localized] == 0 } {

        puts $CHECK_OUTPUT "not localized"
        puts $CHECK_OUTPUT "macro: >$macro_messages_list($i,macro)<"
        puts $CHECK_OUTPUT "file : >$macro_messages_list($i,file)<"
        lappend not_localized $i
        set back [ start_remote_prog "es-ergb01-01" $CHECK_USER "tcsh" " -c \"cd $CHECK_SOURCE_DIR ; grep $macro_messages_list($i,macro) \$C \$H\"" ] 
        puts $back
        if { [ string first "\.c:" $back ] >= 0 } {
           puts $CHECK_OUTPUT "used in C file !!!"
        } else {
           set f_d [open "/tmp/unused_macros.txt" "a"]
           puts $f_d "\nnot used in C-File !!!"
           puts $f_d "macro: $macro_messages_list($i,macro)"    
           puts $f_d "file : $macro_messages_list($i,file)"
           close $f_d
        }
     }
  }

  set f_d [open "/tmp/unused_macros.txt" "a"]

  puts $f_d "not localized messages:"
  puts $f_d "======================="
  foreach mes $not_localized {
     puts $f_d "\nmacro  : $macro_messages_list($mes,macro)"
     puts $f_d "file   : $macro_messages_list($mes,file)"
     puts $f_d "id     : $macro_messages_list($mes,id)"
     puts $f_d "string : $macro_messages_list($mes,string)"
     puts $f_d "line   : $macro_messages_list($mes)"

  }
  close $f_d

}


proc replace_string { input_str what with {only_count 0}} {
   set msg_text $input_str
   set counter 0

   while { 1 } {
      set first [string first $what $msg_text]
      if { $first >= 0 } {
         set last $first
         incr last [string length $what]
         incr last -1
         set msg_text [ string replace $msg_text $first $last "!!__MY_PLACE_HOLDER__!!" ]
         incr counter 1
      } else {
         if { $only_count != 0 } {
            return $counter
         }
         break;
      }
   }
   while { 1 } {
      set first [string first "!!__MY_PLACE_HOLDER__!!" $msg_text]
      if { $first >= 0 } {
         set last $first
         incr last [string length "!!__MY_PLACE_HOLDER__!!"]
         incr last -1
         set msg_text [ string replace $msg_text $first $last $with ]
      } else {
         return $msg_text
      }
   }
}

#****** gettext_procedures/translate() *****************************************
#  NAME
#     translate() -- get l10ned string
#
#  SYNOPSIS
#     translate { host remove_control_signs is_script no_input_parsing msg_txt 
#     { par1 "" } { par2 ""} { par3 "" } { par4 ""} { par5 ""} { par6 ""} } 
#
#  FUNCTION
#     This procedure returns the given string localized to the used language
#
#  INPUTS
#     host                 - host used for infotext call
#     remove_control_signs - if 1: remove control signs ( \n \r ...)
#     is_script            - if 1: text is from install script ( not in c source )
#     no_input_parsing     - if 1: don't try to parse input string
#     msg_txt              - text to translate
#     { par1 "" }          - paramter 1 in msg_txt
#     { par2 ""}           - paramter 2 in msg_txt
#     { par3 "" }          - paramter 3 in msg_txt
#     { par4 ""}           - paramter 4 in msg_txt
#     { par5 ""}           - paramter 5 in msg_txt
#     { par6 ""}           - paramter 6 in msg_txt
#
#  RESULT
#     localized string with optional parameters
#
#  EXAMPLE
#     set SHARETREE [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_OBJ_SHARETREE] ]
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc translate { host remove_control_signs is_script no_input_parsing msg_txt { par1 "" } { par2 ""} { par3 "" } { par4 ""} { par5 ""} { par6 ""} } {

   global CHECK_OUTPUT CHECK_PRODUCT_ROOT CHECK_USER l10n_raw_cache l10n_install_cache

#   puts $CHECK_OUTPUT "cleaning l10n cache buffers ..."
#   if { [ info exists l10n_raw_cache ] } {
#      unset l10n_raw_cache
#   }
#   if { [ info exists l10n_install_cache ] } {
#      unset l10n_install_cache
#   }


   set msg_text $msg_txt
   if { $no_input_parsing != 1 } {
      set msg_text [replace_string $msg_text "\n" "\\n"]
      set msg_text [replace_string $msg_text "\\\"" "__QUOTE_DUMMY_"]
      set msg_text [replace_string $msg_text "\"" "\\\""]
      set msg_text [replace_string $msg_text "__QUOTE_DUMMY_" "\\\""]
   }

#   puts $CHECK_OUTPUT [eval exec "echo \"$msg_text\" | /usr/bin/od -c"]
#   puts $CHECK_OUTPUT "translation of:\n\"$msg_text\""
#   puts $CHECK_OUTPUT [eval exec "echo \"$msg_text\" | /usr/bin/od -c"]

   set arch_string [resolve_arch $host]


   if { $is_script == 0 } {
      set msg_text [replace_string $msg_text "\\\$" "__DOLLAR_DUMMY_"]
      set msg_text [replace_string $msg_text "\$" "\\\$"]
      set msg_text [replace_string $msg_text "\\'" "'"]

      set msg_text [replace_string $msg_text "__DOLLAR_DUMMY_" "\\\$"]
      if { [ info exists l10n_raw_cache($msg_text) ] } {
          set back $l10n_raw_cache($msg_text)
          set prg_exit_state 0
          debug_puts "reading message from l10n raw cache ..."
      } else {
          set back [start_remote_prog $host $CHECK_USER $CHECK_PRODUCT_ROOT/utilbin/$arch_string/infotext "-raw -__eoc__ \"$msg_text\""]
          set l10n_raw_cache($msg_text) $back
          debug_puts "adding message to l10n raw cache ..." 
      }
   } else {
      set num_params [ replace_string $msg_text "%s" "" 1]
      set para_num 1
      set parameter_list ""
      while { $num_params > 0 } {
         set parameter "PAR_$para_num"
         append parameter_list "$parameter "
         incr num_params -1
         incr para_num 1
      }
      if { [ info exists l10n_install_cache($msg_text) ] } {
         set back $l10n_install_cache($msg_text)
         set prg_exit_state 0
         debug_puts "reading message from l10n install cache ..."
      } else {
         set back [start_remote_prog $host $CHECK_USER $CHECK_PRODUCT_ROOT/utilbin/$arch_string/infotext "-n -__eoc__ \"$msg_text\" $parameter_list"]
         set l10n_install_cache($msg_text) $back
         debug_puts "adding message to l10n install cache ..." 
      }
   }
   if { $prg_exit_state == 0} { 
#      puts $CHECK_OUTPUT "---"
      set trans_mes "$back"
#      puts $CHECK_OUTPUT "start_remote_prog returned:\n\"$back\""
#      puts $CHECK_OUTPUT [eval exec "echo \"$trans_mes\" | /usr/bin/od -c"]
#      wait_for_enter 
      if { $remove_control_signs != 0 } {
         set trans_mes [replace_string $trans_mes "\r" ""]
         set trans_mes [replace_string $trans_mes "\n" ""]
      }
      if { $no_input_parsing != 1 } {
         set trans_mes [replace_string $trans_mes "\[" "\\\["]
         set trans_mes [replace_string $trans_mes "\]" "\\\]"]
      }



#      puts $CHECK_OUTPUT [eval exec "echo \"$trans_mes\" | /usr/bin/od -c"]
      set msg_text $trans_mes
   } else {
      add_proc_error "translate" -1 "gettext returned error:\n--$back\n--"
   }
   # search for %....s specifiers and replace them with parameters (%n$s)
   if { $par1 != "" } {
      set p_numb 1
      while { [set s_specifier [ string first "%" $msg_text]] >= 0 } {
         set spec_start_string [string range $msg_text $s_specifier end]
         set spec_end__string  [string first "s" $spec_start_string]
         set spec_end__decimal [string first "d" $spec_start_string]
         set spec_end__character [string first "c" $spec_start_string]
         if { $spec_end__character >= 0 } {
            if { $spec_end__character < $spec_end__decimal  } {
               set spec_end__decimal $spec_end__character
            }
         }

         if { $spec_end__string >= 0 && $spec_end__decimal >= 0 } {
            if { $spec_end__string < $spec_end__decimal } {
               set spec_end $spec_end__string
            } else {
               set spec_end $spec_end__decimal
            }
         } else {
            if { $spec_end__string >= 0 } {
               set spec_end $spec_end__string
            } 
            if { $spec_end__decimal >= 0 } {
               set spec_end $spec_end__decimal
            }
         }
         set spec_string [ string range $spec_start_string 0 $spec_end] 
         incr spec_end 1 
#         puts $CHECK_OUTPUT "specifier($p_numb): \"$spec_string\""
         if { [string first "\$" $spec_string] >= 0 } {
            set p_numb [string range $spec_string 1 1]
#            puts $CHECK_OUTPUT "--> new parameter order, parameter number is $p_numb"
         }
         incr s_specifier -1
         set new_msg_text [string range $msg_text 0 $s_specifier ]
         incr s_specifier 1 
         append new_msg_text "PAR_$p_numb"
         incr spec_end $s_specifier
         append new_msg_text [string range $msg_text $spec_end end ]
         set msg_text $new_msg_text
         incr p_numb 1
      }
   }
   set msg_text [replace_string $msg_text "PAR_1" $par1]
   set msg_text [replace_string $msg_text "PAR_2" $par2]
   set msg_text [replace_string $msg_text "PAR_3" $par3]
   set msg_text [replace_string $msg_text "PAR_4" $par4]
   set msg_text [replace_string $msg_text "PAR_5" $par5]
   set msg_text [replace_string $msg_text "PAR_6" $par6]

#   puts $CHECK_OUTPUT "returned(final):\n\"$msg_text\"" 
   return $msg_text
}

#****** check/perform_simple_l10n_test() ***************************************
#  NAME
#     perform_simple_l10n_test() -- check minimal l10n settings
#
#  SYNOPSIS
#     perform_simple_l10n_test { } 
#
#  FUNCTION
#     This will try to get the translated version of an message string
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc perform_simple_l10n_test { } {

   global CHECK_CORE_MASTER CHECK_USER CHECK_L10N ts_host_config ts_config
   global CHECK_OUTPUT CHECK_HOST CHECK_ARCH l10n_raw_cache
   puts $CHECK_OUTPUT ""
   flush $CHECK_OUTPUT

   set mem_it $CHECK_L10N


   set CHECK_L10N 0
   if { [ info exists l10n_raw_cache] } {
      unset l10n_raw_cache
   }
   set no_l10n  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro SGE_INFOTEXT_TESTSTRING_S_L10N ] " $CHECK_USER " ]
   set CHECK_L10N 1
   unset l10n_raw_cache
   set with_l10n  [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro SGE_INFOTEXT_TESTSTRING_S_L10N ] " $CHECK_USER " ]

#   puts $CHECK_OUTPUT [translate $CHECK_CORE_MASTER 0 0 0 [sge_macro MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER ]]

   puts $CHECK_OUTPUT "\n------------------------------------------------------------------------\n"
   puts $CHECK_OUTPUT $with_l10n
   puts $CHECK_OUTPUT "------------------------------------------------------------------------"
   set CHECK_L10N $mem_it

   if { [ string compare $no_l10n $with_l10n ] == 0 } {
      add_proc_error "perform_simple_l10n_test" -1 "localization (l10n) error:\nIs the locale directory available?"
      return -1
   } 
   return 0
}


# if { [info exists argc ] != 0 } {
#    set TS_ROOT ""
#    set procedure ""
#    for { set i 0 } { $i < $argc } { incr i } {
#       if {$i == 0} { set TS_ROOT [lindex $argv $i] }
#       if {$i == 1} { set procedure [lindex $argv $i] }
#    }
#    if { $argc == 0 } {
#       puts "usage:\ngettext_procedures.tcl <CHECK_TESTSUITE_ROOT> <proc> no_main <testsuite params>"
#       puts "options:"
#       puts "CHECK_TESTSUITE_ROOT -  path to TESTSUITE directory"
#       puts "proc                 -  procedure from this file with parameters"
#       puts "no_main              -  used to source testsuite file (check.exp)"
#       puts "testsuite params     -  any testsuite command option (from file check.exp)"
#       puts "                        testsuite params: file <path>/defaults.sav is needed"
#    } else {
#       source "$TS_ROOT/check.exp"
#       if { $be_quiet == 0 } {
#           puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
#           puts $CHECK_OUTPUT "calling \"$procedure\" ..."
#       }
#       set result [ eval $procedure ]
#       puts $result 
#       flush $CHECK_OUTPUT
#    }
# }

