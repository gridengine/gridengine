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

global file_procedure_logfile_wait_sp_id

#                                                             max. column:     |
#****** file_procedures/test_file() ******
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

#                                                             max. column:     |
#****** file_procedures/get_dir_names() ******
# 
#  NAME
#     get_dir_names -- return all subdirectory names 
#
#  SYNOPSIS
#     get_dir_names { path } 
#
#  FUNCTION
#     read in directory and return a list of subdirectory names 
#
#  INPUTS
#     path - path to read in 
#
#  RESULT
#     list of subdirectory names 
#
#  EXAMPLE
#     set dirs [ get_dir_names /tmp ] 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     file_procedures/get_file_names
#*******************************
proc get_dir_names { path } {

  catch {glob "$path/*"} r1; 
  set r2 ""; 
  foreach filename $r1 {
     if { ( [file isdirectory $filename] == 1 ) && ( [string compare [file tail $filename] "CVS"] != 0 ) } {
        lappend r2 [file tail $filename]
     }
  }
  return $r2;
}

#****** file_procedures/get_tmp_directory_name() *******************************
#  NAME
#     get_tmp_directory_name() -- returns temporary directory path
#
#  SYNOPSIS
#     get_tmp_directory_name { { hostname "" } { type "default" } 
#     { dir_ext "tmp" } } { not_in_results 0 }
#
#  FUNCTION
#     Generates a temporary usable directory name (full path). The parameters
#     are used to define substrings of the directory name. The path
#     is located in the testsuite main results directory or in "tmp" if the
#     testsuite results directory is not accessable.
#
#  INPUTS
#     { hostname "" }      - a hostname substring
#     { type "default" }   - a type substring
#     { dir_ext "tmp" }    - a extension substring
#     { not_in_results 0 } - if not 0: generate path in /tmp
#
#  RESULT
#     full path string of a directory
#
#  SEE ALSO
#     file_procedures/get_tmp_file_name()
#*******************************************************************************
proc get_tmp_directory_name { { hostname "" } { type "default" } { dir_ext "tmp" } { not_in_results 0 } } {
   global CHECK_MAIN_RESULTS_DIR CHECK_HOST CHECK_USER CHECK_OUTPUT

   if { $hostname == "" } {
      set hostname $CHECK_HOST
   }

   if { [ file isdirectory $CHECK_MAIN_RESULTS_DIR ] != 1 || $not_in_results != 0 } {
     set file_name "/tmp/${CHECK_USER}_${hostname}_${type}_[timestamp]_${dir_ext}"
   } else {
     set file_name "$CHECK_MAIN_RESULTS_DIR/${CHECK_USER}_${hostname}_${type}_[timestamp]_${dir_ext}"
   }
   return $file_name
}

#****** file_procedures/get_tmp_file_name() ************************************
#  NAME
#     get_tmp_file_name() -- generate temporary filename 
#
#  SYNOPSIS
#     get_tmp_file_name { { hostname "" } { type "default" } { file_ext "tmp" } 
#     } { not_in_results 0 }
#
#  FUNCTION
#     Generates a temporary usable file name (full path). The parameters
#     are used to define substrings of the file name. The path
#     is located in the testsuite main results directory or in "tmp" if the
#     testsuite results directory is not accessable.
#
#     The file is automatically erased when:
#
#        a) The testsuite menu() procedure is called
#        b) A new test ( or testlevel) is started
#
#     So if the caller is generating this file, he as not to delete it.
#
#  INPUTS
#     { hostname "" }      - a hostname substring
#     { type "default" }   - a type substring
#     { file_ext "tmp" }   - a extension substring
#     { not_in_results 0 } - if not 0: generate file in /tmp
#
#  RESULT
#    a filename string ( absolute path )
#
#  SEE ALSO
#     file_procedures/get_tmp_directory_name()
#*******************************************************************************
proc get_tmp_file_name { { hostname "" } { type "default" } { file_ext "tmp" } { not_in_results 0 } } {
   
   global CHECK_MAIN_RESULTS_DIR CHECK_HOST CHECK_USER CHECK_OUTPUT

   if { $hostname == "" } {
      set hostname $CHECK_HOST
   }


   while { 1 } {
      if { [ file isdirectory $CHECK_MAIN_RESULTS_DIR ] != 1  || $not_in_results != 0 } {
        set file_name "/tmp/${CHECK_USER}_${hostname}_${type}_[timestamp].${file_ext}"
      } else {
        set file_name "$CHECK_MAIN_RESULTS_DIR/${CHECK_USER}_${hostname}_${type}_[timestamp].${file_ext}"
      }
    
      # break loop when file is not existing ( when timestamp has increased )  
      if { [ file isfile $file_name] != 1 } {
         break
      }
      sleep 1
   }


   delete_file_at_startup $file_name
 
   return $file_name
}

#****** file_procedures/dump_array_data() **************************************
#  NAME
#     dump_array_data() -- dump array data to $CHECK_OUTPUT
#
#  SYNOPSIS
#     dump_array_data { obj_name obj } 
#
#  FUNCTION
#     This procedure dumps all array data to $CHECK_OUTPUT
#
#  INPUTS
#     obj_name - data object name (used for output)
#     obj      - array name
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc dump_array_data { obj_name obj } {
   global CHECK_OUTPUT
   upvar $obj data

   set names [array names data]
   set names [lsort $names]
   foreach elem $names {
      puts $CHECK_OUTPUT "$obj_name->$elem=$data($elem)"
   }
}

#****** file_procedures/convert_spool_file_to_html() ***************************
#  NAME
#     convert_spool_file_to_html() -- convert array data in spool file to html 
#
#  SYNOPSIS
#     convert_spool_file_to_html { spoolfile htmlfile { just_return_content 0 }} 
#
#  FUNCTION
#     This procedure generates a html output file from an array spool file.
#     Use the procedure spool_array_to_file() to generate a array spool file.
#
#  INPUTS
#     spoolfile                 - spool file directory path
#     htmlfile                  - output file directory path
#     { just_return_content 0 } - if 1: return just html content
#                               - if 0: create file and return html content
#
#  SEE ALSO
#     file_procedures/generate_html_file()
#     file_procedures/create_html_table()
#     file_procedures/create_html_link()
#     file_procedures/create_html_text()
#*******************************************************************************
proc convert_spool_file_to_html { spoolfile htmlfile { just_return_content 0 }} {
   global CHECK_OUTPUT

   set content ""

   # read in spool file
   read_file $spoolfile file_dat
   
   # get all stored obj_names
   set obj_names [get_all_obj_names file_dat]

   foreach obj $obj_names {
      set obj_start [search_for_obj_start file_dat $obj]
      set obj_end   [search_for_obj_end file_dat $obj]
      for { set i $obj_start } { $i <= $obj_end  } { incr i 1 } {
         incr i 1
         set spec [unpack_data_line $file_dat($i)]
         incr i 1
         set spec_data [unpack_data_line $file_dat($i)]
         set obj_data($spec) $spec_data
      }
      if { $just_return_content == 0 } {
         append content [create_html_text "Object name: $obj" ]
      }
      set obj_names [array names obj_data]
      set obj_names [lsort $obj_names]
      set obj_names_count [llength $obj_names]
      set table(COLS) 2
      set table(ROWS) $obj_names_count
      for { set tb 1 } { $tb <= $obj_names_count } { incr tb 1 } {
         set obj_name_index [ expr ( $tb - 1 ) ]
         set obj_name [lindex $obj_names $obj_name_index]
         set table($tb,BGCOLOR) "#3366FF"
         set table($tb,FNCOLOR) "#66FFFF"    
         set table($tb,1) $obj_name
         set table($tb,2) [ format_output "" 75 $obj_data($obj_name)]
      }
      append content [create_html_table table]
      unset table
      
      dump_array_data $obj obj_data
      unset obj_data
   }
   if { $just_return_content == 0 } {
      return [generate_html_file $htmlfile "Object Dump" $content 1]
   } else {
      return $content
   }
}

#****** file_procedures/spool_array_to_file() **********************************
#  NAME
#     spool_array_to_file() -- spool array data to array spool file
#
#  SYNOPSIS
#     spool_array_to_file { filename obj_name array_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     filename   - file for data spooling
#     obj_name   - file object name of error
#     array_name - array to spool
#
#  RESULT
#     number of changed values 
#
#  SEE ALSO
#     file_procedures/read_array_from_file()
#*******************************************************************************
proc spool_array_to_file { filename obj_name array_name } {

   global CHECK_OUTPUT 
   upvar $array_name data

   puts $CHECK_OUTPUT "saving object \"$obj_name\" ..."
   set changes 0
   

   # read in file
   read_file $filename file_dat
   
   # get all stored obj_names
   set obj_names [get_all_obj_names file_dat]
   set new_file_dat ""
   unset new_file_dat 
   set new_file_dat(0) 0
   set act_line 1
   # store all objects in file_array
   foreach obj $obj_names {
      if { [string compare $obj $obj_name] == 0 } {
         # here we ignore old object data, but load it into an array
         # puts $CHECK_OUTPUT "ignoring old \"$obj_name\" object data ..."
         set obj_start [search_for_obj_start file_dat $obj]
         set obj_end   [search_for_obj_end file_dat $obj]
         for { set i $obj_start } { $i <= $obj_end  } { incr i 1 } {
            incr i 1
            set spec [unpack_data_line $file_dat($i)]
            incr i 1
            set spec_data [unpack_data_line $file_dat($i)]
            set old_data($spec) $spec_data
         }     
      } else {
         # here we save all other objects    
         # puts $CHECK_OUTPUT "saving \"$obj\" object data ..."
         set obj_start [search_for_obj_start file_dat $obj]
         set obj_end   [search_for_obj_end file_dat $obj]
         incr obj_start -1
         incr obj_end 1
         for { set i $obj_start } { $i <= $obj_end  } { incr i 1 } {
            set new_file_dat($act_line) $file_dat($i)
            set new_file_dat(0) $act_line
            incr act_line 1
         } 
      }
   }

   # puts $CHECK_OUTPUT "saving new data ..."
   # here we store the new data
   set data_count [array names data]
   set data_count [llength $data_count]
   if { $data_count > 0 } {
      set new_file_dat($act_line) "OBJ_START:$obj_name:"
      incr act_line 1
      set data_specs [array names data]
      set data_specs [lsort $data_specs]
      foreach spec $data_specs {
         # puts $CHECK_OUTPUT "saving \"$obj_name->$spec\" ..."
         set new_file_dat($act_line) "####### $spec #######"
         incr act_line 1
         set new_file_dat($act_line) [pack_data_line $spec]
         incr act_line 1
         set new_file_dat($act_line) [pack_data_line $data($spec)]
         incr act_line 1
      }
      set new_file_dat($act_line) "OBJ_END:$obj_name:"
      set new_file_dat(0) $act_line
   }

   # now write tmp new file_dat
   save_file $filename.tmp new_file_dat

   # shall we compare objects ? 
   set new_elems ""
   set removed_elems ""



   if { [ info exists old_data] } {
      set array_names_old [array names old_data]
      set array_names_new [array names data]
      set new_elems ""
      set removed_elems ""

      foreach new $array_names_new {
         set found 0
         foreach old $array_names_old {
            if { [string compare $old $new] == 0 } {
               set found 1
            }
         }
         if { $found == 0 } {
            lappend new_elems $new
         }
      }
      foreach old $array_names_old {
         set found 0
         foreach new $array_names_new {
            if { [string compare $old $new] == 0 } {
               set found 1
            }
         }
         if { $found == 0 } {
            lappend removed_elems $old
         }
      }
   } else {
      set array_names_new [array names data]
      foreach elem $array_names_new {
         lappend new_elems $elem
      }
   }
   
#   puts $CHECK_OUTPUT "removed: $removed_elems"
#   puts $CHECK_OUTPUT "added  : $new_elems"

   incr changes [llength $removed_elems]
   incr changes [llength $new_elems]



   # delete old file, rename new file
   if { [file isfile $filename.old] } {
      file delete $filename.old
   }
   if { [file isfile $filename]} {
      file rename $filename $filename.old
   }
   if { [file isfile $filename.tmp]} {
      file rename $filename.tmp $filename
      file delete $filename.tmp
   }
   return $changes
}

#****** file_procedures/save_file() ********************************************
#  NAME
#     save_file() -- saving array file data to file
#
#  SYNOPSIS
#     save_file { filename array_name } 
#
#  FUNCTION
#     This procedure saves the data in the array to the file
#
#  INPUTS
#     filename   - filename
#     array_name - name of array to save
#
#  EXAMPLE
#     set data(0) 1
#     set data(1) "the file will have this line"
#     save_file myfile.txt data
#
#  SEE ALSO
#     file_procedures/read_file()
#*******************************************************************************
proc save_file { filename array_name } {
   global CHECK_OUTPUT
   upvar  $array_name data

   
   set file [open $filename "w"]
   set last_line $data(0)
   puts $CHECK_OUTPUT "saving file \"$filename\""
   for { set i 1 } { $i <= $last_line } { incr i 1 } {
#      puts -nonewline $CHECK_OUTPUT [washing_machine $i]
#      flush $CHECK_OUTPUT
      puts $file $data($i)
   }
   close $file
   puts $CHECK_OUTPUT "\ndone"
}

#****** file_procedures/read_file() ********************************************
#  NAME
#     read_file() -- read fill into array (line by line)
#
#  SYNOPSIS
#     read_file { filename array_name } 
#
#  FUNCTION
#     This procedure reads the content of the given file and saves the lines
#     into the array
#
#  INPUTS
#     filename   - file
#     array_name - name of array to store file content
#
#  EXAMPLE
#     read_file myfile.txt data
#     set nr_of_lines $data(0)
#     for { set i 0 } { $i != nr_of_lines } { incr i 1 } {
#        puts $data($i)
#     }
#
#  SEE ALSO
#     file_procedures/save_file()
#*******************************************************************************
proc read_file { filename array_name } {

   global CHECK_OUTPUT
   upvar  $array_name data
   
   if { [file isfile $filename] != 1 } {
      set data(0) 0
      puts $CHECK_OUTPUT "\"$filename\" is not a file"
      return
   }
   set file [open $filename "r"]
   set x 1
   while { [gets $file line] >= 0 } {
#       puts -nonewline $CHECK_OUTPUT [washing_machine $x]
#       flush $CHECK_OUTPUT
       set data($x) $line
       incr x 1
   }
   close $file
   incr x -1
   set data(0) $x
   debug_puts "file \"$filename\" has $x lines"
}


#****** file_procedures/get_all_obj_names() ************************************
#  NAME
#     get_all_obj_names() -- return object(array) names from array spool file
#
#  SYNOPSIS
#     get_all_obj_names { file_array } 
#
#  FUNCTION
#     Returns all object (array) names from an array spool file
#
#  INPUTS
#     file_array - array name of file data array (see read_file())
#
#  SEE ALSO
#     file_procedures/read_file()
#*******************************************************************************
proc get_all_obj_names { file_array } {
   global CHECK_OUTPUT
   upvar $file_array file_dat

   set obj_names "" 
  
   for { set i 1 } { $i <= $file_dat(0)  } { incr i 1 } {
      set line $file_dat($i)
      if { [string first "OBJ_START:" $line ] == 0 } {
         set start [string first ":" $line ]
         set end   [string last ":" $line ] 
         incr start 1
         incr end -1
         set found_job_name [string range $line $start $end]
         lappend obj_names $found_job_name
      }
  }
  return $obj_names
}

#****** file_procedures/search_for_obj_start() *********************************
#  NAME
#     search_for_obj_start() --  search line of object start in file array
#
#  SYNOPSIS
#     search_for_obj_start { file_array obj_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     file_array - name of file array (see save_file())
#     obj_name   - name of object
#
#  RESULT
#     line number or -1 on error
#
#  SEE ALSO
#     file_procedures/search_for_obj_end()
#     file_procedures/save_file()
#*******************************************************************************
proc search_for_obj_start { file_array obj_name } {
   global CHECK_OUTPUT
   upvar $file_array file_dat
   
   for { set i 1 } { $i <= $file_dat(0)  } { incr i 1 } {
      set line $file_dat($i)
      if { [string first "OBJ_START:" $line ] == 0 } {
         set start [string first ":" $line ]
         set end   [string last ":" $line ] 
         incr start 1
         incr end -1
         set found_job_name [string range $line $start $end]
         if { [string compare $obj_name $found_job_name] == 0 } {
            incr i 1
            return $i
         }
      }
  }
  return -1
}

#****** file_procedures/search_for_obj_end() ***********************************
#  NAME
#     search_for_obj_end() -- search line of object end in file array
#
#  SYNOPSIS
#     search_for_obj_end { file_array obj_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     file_array - name of file array (see save_file())
#     obj_name   - name of object 
#
#  RESULT
#     line number or -1 on error
#
#  SEE ALSO
#     file_procedures/search_for_obj_start()
#     file_procedures/save_file()
#*******************************************************************************
proc search_for_obj_end { file_array obj_name } {
   global CHECK_OUTPUT
   upvar $file_array file_dat
   
   for { set i 1 } { $i <= $file_dat(0)  } { incr i 1 } {
      set line $file_dat($i)
      if { [string first "OBJ_END:" $line ] == 0 } {
         set start [string first ":" $line ]
         set end   [string last ":" $line ] 
         incr start 1
         incr end -1
         set found_job_name [string range $line $start $end]
         if { [string compare $obj_name $found_job_name] == 0 } {
            incr i -1
            return $i
         }
      }
  }
  return -1
}

#****** file_procedures/unpack_data_line() *************************************
#  NAME
#     unpack_data_line() -- convert file data line to orignial data
#
#  SYNOPSIS
#     unpack_data_line { line } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     line - data line in file
#
#  SEE ALSO
#     file_procedures/pack_data_line()
#*******************************************************************************
proc unpack_data_line { line } {
   set start [string first ":" $line]
   incr start 1
   set end [string last ":" $line]
   incr end -1
   set data [string range $line $start $end]
   set data [replace_string $data "_TS_NEW_LINE_TS_" "\n"]
   set data [replace_string $data "_TS_CR_RETURN_TS_" "\r"]
   return $data
}

#****** file_procedures/pack_data_line() ***************************************
#  NAME
#     pack_data_line() -- convert data line to file
#
#  SYNOPSIS
#     pack_data_line { line } 
#
#  FUNCTION
#     do transformation of data to ensure correct data saving
#
#  INPUTS
#     line - data line in file
#
#  SEE ALSO
#     file_procedures/unpack_data_line()
#*******************************************************************************
proc pack_data_line { line } {
   set data ":$line:"
   set data [replace_string $data "\n" "_TS_NEW_LINE_TS_"]
   set data [replace_string $data "\r" "_TS_CR_RETURN_TS_"]
   return $data
}

#****** file_procedures/read_array_from_file() *********************************
#  NAME
#     read_array_from_file() -- read array data from array spool file
#
#  SYNOPSIS
#     read_array_from_file { filename obj_name array_name 
#     { enable_washing_machine 0 } } 
#
#  FUNCTION
#     This procedure will read the content of an array spool file and store it
#     into a tcl array.
#
#  INPUTS
#     filename                     - filename of array spool file
#     obj_name                     - name of object in array spool file
#     array_name                   - array name to store data
#     { enable_washing_machine 0 } - show washing machine
#
#  RESULT
#     0 on success, -1 on error
#
#  SEE ALSO
#      file_procedures/spool_array_to_file()
#*******************************************************************************
proc read_array_from_file { filename obj_name array_name { enable_washing_machine 0 } } {
  global CHECK_OUTPUT
  upvar $array_name data

  read_file $filename file_dat
  set obj_start [search_for_obj_start file_dat $obj_name]
  if { $obj_start < 0 } {
     return -1
  }
  set obj_end   [search_for_obj_end file_dat $obj_name]
  if { $obj_end < 0 } {
     return -1
  }
  set wcount 0
  set time 0
  for { set i $obj_start } { $i <= $obj_end  } { incr i 1 } {
     incr i 1
     set spec [unpack_data_line $file_dat($i)]
     incr i 1
     set spec_data [unpack_data_line $file_dat($i)]
     set data($spec) $spec_data
     if { $enable_washing_machine != 0 && $wcount > 20 } {
        puts -nonewline $CHECK_OUTPUT "\r reading ...    \r reading ... [washing_machine $time 1]\r"
        flush $CHECK_OUTPUT
        set wcount 0
        incr time 1
     }
     incr wcount 1
  }
  return 0
}


#****** file_procedures/read_array_from_file_data() ****************************
#  NAME
#     read_array_from_file_data() -- read array data from file data array
#
#  SYNOPSIS
#     read_array_from_file_data { file_data obj_name array_name 
#     { enable_washing_machine 0 } } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     file_data                    - file data object name
#     obj_name                     - object to read from file data
#     array_name                   - array name to store object
#     { enable_washing_machine 0 } - optional: display washing machine
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
#*******************************************************************************
proc read_array_from_file_data { file_data obj_name array_name { enable_washing_machine 0 } } {
  global CHECK_OUTPUT
  upvar $array_name data
  upvar $file_data file_dat

  set obj_start [search_for_obj_start file_dat $obj_name]
  if { $obj_start < 0 } {
     return -1
  }
  set obj_end   [search_for_obj_end file_dat $obj_name]
  if { $obj_end < 0 } {
     return -1
  }
  set wcount 0
  set time 0
  for { set i $obj_start } { $i <= $obj_end  } { incr i 1 } {
     incr i 1
     set spec [unpack_data_line $file_dat($i)]
     incr i 1
     set spec_data [unpack_data_line $file_dat($i)]
     set data($spec) $spec_data
     if { $enable_washing_machine != 0 && $wcount > 20 } {
        puts -nonewline $CHECK_OUTPUT "\r reading ...    \r reading ... [washing_machine $time 1]\r"
        flush $CHECK_OUTPUT
        set wcount 0
        incr time 1
     }
     incr wcount 1
  }
  return 0
}





#****** file_procedures/get_all_subdirectories() *******************************
#  NAME
#     get_all_subdirectories() -- returns all subdirectories in path 
#
#  SYNOPSIS
#     get_all_subdirectories { path } 
#
#  FUNCTION
#     This procedure returns a list of all sub directories (recursive) in
#     given path
#
#  INPUTS
#     path - root directory path
#
#  RESULT
#     list of subdirectories
#
#*******************************************************************************
proc get_all_subdirectories { path } {
  set directories ""
  set files [get_file_names $path] 
  set dirs [get_dir_names $path]
  
  foreach elem $dirs {
     lappend directories "$elem"
  }
  
  foreach element $dirs {
     set sub_dirs [ get_all_subdirectories "$path/$element"]
     foreach elem $sub_dirs {
        lappend directories "$element/$elem"
     }
  }
  return $directories
}


# get all file names of path
#                                                             max. column:     |
#****** file_procedures/get_file_names() ******
# 
#  NAME
#     get_file_names -- return all file names of directory 
#
#  SYNOPSIS
#     get_file_names { path {ext "*"} } 
#
#  FUNCTION
#     read in directory and return a list of file names in this directory 
#
#  INPUTS
#     path - path to read in (directory) 
#     ext  - file extension (default "*")
#
#  RESULT
#     list of file names 
#
#  EXAMPLE
#     set files [ get_file_names /tmp ] 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     file_procedures/get_dir_names
#*******************************
proc get_file_names { path {ext "*"} } {
  catch {glob "$path/$ext"} r1;
  set r2 ""; 
  foreach filename $r1 {
     if { [file isfile $filename] == 1 } {
        lappend r2 [file tail $filename]
     }
  }
  return $r2;
}


#****** file_procedures/generate_html_file() ***********************************
#  NAME
#     generate_html_file() -- generate html file
#
#  SYNOPSIS
#     generate_html_file { file headliner content { return_text 0 } } 
#
#  FUNCTION
#     This procedure creates the html file with the given headline and
#     text content.
#
#  INPUTS
#     file              - html file name to create
#     headliner         - headline text
#     content           - html body
#     { return_text 0 } - if not 0: return file content
#
#  SEE ALSO
#     file_procedures/generate_html_file()
#     file_procedures/create_html_table()
#     file_procedures/create_html_link()
#     file_procedures/create_html_text()
#*******************************************************************************
proc generate_html_file { file headliner content { return_text 0 } } {

   global CHECK_USER

   set output ""

   set catch_return [ catch {
      set h_file [ open "$file" "w" ]
   } ]
   if { $catch_return != 0 } {
      add_proc_error "generate_html_file" "-1" "could not open file $file for writing"
      return
   }
   lappend output "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">"
   lappend output "<html>"
   lappend output "<head>"
   lappend output "   <meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">"
   lappend output "   <meta name=\"Author\" content=\"Grid Engine Testsuite - user ${CHECK_USER}\">"
   lappend output "   <meta name=\"GENERATOR\" content=\"unknown\">"
   lappend output "</head>"
   lappend output "<body text=\"#000000\" bgcolor=\"#FFFFFF\" link=\"#CCCCCC\" vlink=\"#999999\" alink=\"#993300\">"
   lappend output ""
   lappend output "<hr WIDTH=\"100%\">"
   lappend output "<center><font size=+2>$headliner</font></center>"
   lappend output ""
   lappend output "<hr WIDTH=\"100%\">"
   lappend output "<br>&nbsp;"
   lappend output "<br>&nbsp;"
   lappend output ""
   lappend output "$content"
   lappend output ""
   lappend output "</body>"
   lappend output "</html>"
   foreach line $output {
      puts $h_file $line
   }
   flush $h_file
   close $h_file
   
   if { $return_text != 0 } {
      set return_value ""
      foreach line $output {
         append return_value "$line\n"
      }
      return $return_value
   }
}

#****** file_procedures/create_html_table() ************************************
#  NAME
#     create_html_table() -- returns tcl array in html format
#
#  SYNOPSIS
#     create_html_table { array_name } 
#
#  FUNCTION
#     This procedure tries to transform the given array into an html table
#
#  INPUTS
#     array_name - table content
#
#     table(COLS) = nr. of columns
#     table(ROWS) = nr. of rows
#     table(ROW number,BGCOLOR) = Background color for row
#     table(ROW number,FNCOLOR) = Fontcolor of row
#     table(ROW number,1 up to $COLS) = content
#
#
#  RESULT
#     html format
#
#  EXAMPLE
#     set test_table(COLS) 2
#     set test_table(ROWS) 3
#     set test_table(1,BGCOLOR) "#3366FF"
#     set test_table(1,FNCOLOR) "#66FFFF"
#     set test_table(1,1) "Host"
#     set test_table(1,2) "State"
#   
#     set test_table(2,BGCOLOR) "#009900"
#     set test_table(2,FNCOLOR) "#FFFFFF"
#     set test_table(2,1) "host1"
#     set test_table(2,2) "ok"
#     
#     set test_table(3,BGCOLOR) "#CC0000"
#     set test_table(3,FNCOLOR) "#FFFFFF"
#     set test_table(3,1) "host2"
#     set test_table(3,2) [create_html_link "linktext" "test.html"]
#   
#     set my_content    [ create_html_text "Date: [exec date]" ]
#     append my_content [ create_html_text "some text ..." ]
#     append my_content [ create_html_table test_table ]
#     generate_html_file test.html "My first HTML example!!!" $my_content
#
#  SEE ALSO
#     file_procedures/generate_html_file()
#     file_procedures/create_html_table()
#     file_procedures/create_html_link()
#     file_procedures/create_html_text()
#*******************************************************************************
proc create_html_table { array_name { border 0 } { align LEFT } } {
   upvar $array_name table

   set back ""
   append back "\n<center><table BORDER=$border COLS=${table(COLS)} WIDTH=\"80%\" NOSAVE >\n" 
   for {set row 1} { $row <= $table(ROWS) } { incr row 1 } {
      append back "<tr ALIGN=$align VALIGN=CENTER BGCOLOR=\"$table($row,BGCOLOR)\" NOSAVE>\n"
      for {set col 1} { $col <= $table(COLS) } { incr col 1 } {
         if { [ info exists table($row,$col,FNCOLOR) ] } {
            append back "<td NOSAVE><b><font color=\"$table($row,$col,FNCOLOR)\"><font size=+1>$table($row,$col)</font></font></b></td>\n"
         } else {
            append back "<td NOSAVE><b><font color=\"$table($row,FNCOLOR)\"><font size=+1>$table($row,$col)</font></font></b></td>\n"
         }
      }
      append back "</tr>\n"
   }
   append back "</table></center>\n"
   return $back
}

#****** file_procedures/create_html_link() *************************************
#  NAME
#     create_html_link() -- create html link
#
#  SYNOPSIS
#     create_html_link { linktext linkref } 
#
#  FUNCTION
#     This procedure returns a html format for a "link"
#
#  INPUTS
#     linktext - text to display for link
#     linkref  - link to destination
#
#  RESULT
#     html format
#
#  SEE ALSO
#     file_procedures/generate_html_file()
#     file_procedures/create_html_table()
#     file_procedures/create_html_link()
#     file_procedures/create_html_text()
#*******************************************************************************
proc create_html_link { linktext linkref } {
   set back ""

   append back "<a href=\"$linkref\">$linktext</a>" 

   return $back
}

#****** file_procedures/create_html_image() ************************************
#  NAME
#     create_html_image() -- integrate html image
#
#  SYNOPSIS
#     create_html_image { alternative_text path } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     alternative_text - alternative text of image
#     path             - path or link to image
#
#  RESULT
#     html output
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc create_html_image { alternative_text path } {

   set back ""
   append back "<center><img SRC=\"$path\" ALT=\"$alternative_text\" NOSAVE></center>"
   return $back
}

#****** file_procedures/create_html_target() ***********************************
#  NAME
#     create_html_target() -- append html target
#
#  SYNOPSIS
#     create_html_target { target_name } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     target_name - link, name of target
#
#  RESULT
#     html output
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc create_html_target { target_name } {
   set back ""
   append back "<p><a NAME=\"$target_name\"></a>"
   return $back
}

#****** file_procedures/create_html_text() *************************************
#  NAME
#     create_html_text() -- create html text
#
#  SYNOPSIS
#     create_html_text { content { center 0 } } 
#
#  FUNCTION
#     This procedure returns a html format for "text"
#
#  INPUTS
#     content      - text 
#     { center 0 } - if not 0: center text
#
#  RESULT
#     html format
#
#  SEE ALSO
#     file_procedures/generate_html_file()
#     file_procedures/create_html_table()
#     file_procedures/create_html_link()
#     file_procedures/create_html_text()
#*******************************************************************************
proc create_html_text { content { center 0 } } {
   set back ""

   if { $content == "" }  {
      set content "<br>"
   }

   if { $center != 0 } {
      append back "<center>\n"
   }
   append back "\n<p>$content</p>\n"
   if { $center != 0 } {
      append back "</center>\n"
   }
   return $back
}

#                                                             max. column:     |
#****** file_procedures/del_job_files() ******
# 
#  NAME
#     del_job_files -- delete files that conain a specific jobid 
#
#  SYNOPSIS
#     del_job_files { jobid job_output_directory expected_file_count } 
#
#  FUNCTION
#     This function reads in the job_output_directory and is looking for 
#     filenames that contain the given jobid. If after a maximum time of 120 
#     seconds not the number of expected_file_count is reached, a timeout will 
#     happen. After that the files are deleted. 
#
#  INPUTS
#     jobid                - jobid of job which has created the output file 
#     job_output_directory - path to the directory that contains the output files 
#     expected_file_count  - number of output files that are expected 
#
#  RESULT
#     returns the number of deleted files 
#
#  SEE ALSO
#     file_procedures/get_dir_names
#*******************************
proc del_job_files { jobid job_output_directory expected_file_count } {
   global CHECK_OUTPUT

   set del_job_count 0

   set end_time [ expr ( [timestamp] + 120 ) ]   ;# timeout after 120 seconds

   while { [timestamp] < $end_time } {
      set files [ glob -nocomplain $job_output_directory/*e${jobid} $job_output_directory/*o${jobid} $job_output_directory/*${jobid}.* ]
      if { [llength $files] >= $expected_file_count } {
         break
      }
      puts $CHECK_OUTPUT "------------------------------------------------------------------"
      puts $CHECK_OUTPUT "waiting for $expected_file_count jobfiles of job $jobid"
      puts $CHECK_OUTPUT "files found: [llength $files]"
      puts $CHECK_OUTPUT "file list  : \"$files\""
      sleep 1 
   }   
   # ok delete the list 

   puts $CHECK_OUTPUT "job \"$jobid\" has written [llength $files] files"

   if { [ llength $files ] >= 1 } {
     if { [ string length $job_output_directory  ] > 5 } {

        foreach name $files {
           delete_file $name
           debug_puts "del_job_files - file: $name" 
           incr del_job_count 1 
        }
     } else {
        add_proc_error "del_job_files" -1 "job output directory name should have at least 5 characters"
     }
   } 
   return $del_job_count
}


#****** file_procedures/create_shell_script() **********************************
#  NAME
#     create_shell_script() -- create a /bin/sh script file 
#
#  SYNOPSIS
#     create_shell_script { scriptfile host exec_command exec_arguments 
#     {envlist ""} { script_path "/bin/sh" } { no_setup 0 } 
#     { source_settings_file 1 } } 
#
#  FUNCTION
#     This procedure generates a script which will execute the given command. 
#     The script will restore the testsuite and SGE environment first. It will 
#     also echo _start_mark_:(x) and _exit_status_:(x) where x is the exit 
#     value from the started command. 
#
#  INPUTS
#     scriptfile                 - full path and name of scriptfile to generate
#     host                       - host on which the script will run
#     exec_command               - command to execute
#     exec_arguments             - command parameters
#     {envlist ""}               - array with environment settings to export
#     { script_path "/bin/sh" }  - path to script binary (default "/bin/sh")
#     { no_setup 0 }             - if 0 (default): full testsuite framework script
#                                                  initialization
#                                  if not 0:       no testsuite framework init.
#
#     { source_settings_file 1 } - if 1 (default): source the file
#                                                  $SGE_ROOT/default/settings.csh
#                                  if not 1:       don't source settings file
#     { set_shared_lib_path 1 }  - if 1 (default): set shared lib path
#                                  if not 1:       don't set shared lib path
#
#  EXAMPLE
#     set envlist(COLUMNS) 500
#     create_shell_script "/tmp/script.sh" "ps" "-ef" "envlist" 
#
#  SEE ALSO
#     file_procedures/get_dir_names
#     file_procedures/create_path_aliasing_file()
#*******************************************************************************
proc create_shell_script { scriptfile 
                           host 
                           exec_command 
                           exec_arguments 
                           { envlist ""} 
                           { script_path "/bin/sh" } 
                           { no_setup 0 } 
                           { source_settings_file 1 } 
                           { set_shared_lib_path 1 }
                         } {
   global CHECK_OUTPUT CHECK_PRODUCT_TYPE CHECK_COMMD_PORT CHECK_PRODUCT_ROOT
   global CHECK_DEBUG_LEVEL 
 
   upvar $envlist users_env

    
   set_users_environment $host users_env

   set script "no_script"
   set catch_return [ catch {
       set script [ open "$scriptfile" "w" ]
   } ]
   if { $catch_return != 0 } {
      add_proc_error "create_shell_script" "-2" "could not open file $scriptfile for writing"
      return
   }

   # script header
   puts $script "#!${script_path}"
   puts $script "# Automatic generated script from Grid Engine Testsuite"
   puts $script "# The script will execute a special command with arguments"
   puts $script "# and it should be deleted after use. So if this file exists, please delete it"

   if { $no_setup == 0 } { 
      # script command
      puts $script "trap 'echo \"_exit_status_:(1)\"' 0"
      puts $script "umask 022"

      if { $set_shared_lib_path == 1 } {
         get_shared_lib_path $host shared_var shared_value
         puts $script "if \[ x\$$shared_var = x \]; then"
         puts $script "   $shared_var=$shared_value"
         puts $script "   export $shared_var"
         puts $script "else"
         puts $script "   $shared_var=\$$shared_var:$shared_value"
         puts $script "   export $shared_var"
         puts $script "fi"
      }
      if { $source_settings_file == 1 } {
         puts $script "if \[ -f $CHECK_PRODUCT_ROOT/default/common/settings.sh \]; then"
         puts $script "   . $CHECK_PRODUCT_ROOT/default/common/settings.sh"
         puts $script "else"
      }
      puts $script "   unset GRD_ROOT"
      puts $script "   unset CODINE_ROOT"
      puts $script "   unset GRD_CELL"
      puts $script "   unset CODINE_CELL"
      if { [info exists CHECK_COMMD_PORT] } {
         puts $script "   COMMD_PORT=$CHECK_COMMD_PORT"
         puts $script "   export COMMD_PORT"
      }
      if { [info exists CHECK_PRODUCT_ROOT] } {
         puts $script "   SGE_ROOT=$CHECK_PRODUCT_ROOT"
         puts $script "   export SGE_ROOT"
      }
      if { $source_settings_file == 1 } {
         puts $script "fi"
      }
   
      foreach u_env [ array names users_env ] {
         set u_val [set users_env($u_env)] 
         debug_puts "setting $u_env to $u_val"
         puts $script "${u_env}=${u_val}"
         puts $script "export ${u_env}"
      }
      puts $script "echo \"_start_mark_:(\$?)\""
   }

   puts $script "$exec_command $exec_arguments"

   if { $no_setup == 0 } { 
      puts $script "exit_val=\"\$?\""
      puts $script "trap 0"
      puts $script "echo \"_exit_status_:(\$exit_val)\""
      puts $script "echo \"script done.\""
   }
   flush $script
   close $script


   set file_size 0
   while { $file_size == 0 } {
      catch { set file_size [file size "$scriptfile"]}
      if { $file_size == 0 } { 
         puts $CHECK_OUTPUT "--> file size of \"$scriptfile\": $file_size ; waiting for filesize > 0"
         sleep 1
      }
   }
   catch { exec "touch" "$scriptfile" } result
#   puts $CHECK_OUTPUT "touch result: $result"
  
   catch { exec "chmod" "0755" "$scriptfile" } result
#   puts $CHECK_OUTPUT "chmod result: $result"


   if { $CHECK_DEBUG_LEVEL != 0 } {
      set script  [ open "$scriptfile" "r" ]
      debug_puts "*********** script content start *********"
      while { [gets $script line] >= 0 } {
         debug_puts $line
      }
      debug_puts "*********** script content end *********"
      close $script
      if { $CHECK_DEBUG_LEVEL == 2 } {
         wait_for_enter
      }

   }
}



#****** file_procedures/get_file_content() *************************************
#  NAME
#     get_file_content() -- read remote/local file with cat command
#
#  SYNOPSIS
#     get_file_content { host user file { file_a "file_array" } } 
#
#  FUNCTION
#     This procedure fills up the file_array with the content of the given
#     file. file_array(0) contains the number of lines (starting from 1)
#     file_array(1) - file_array($file_array(0)) contains the lines of the 
#     file.
#
#  INPUTS
#     host                    - hostname to connect
#     user                    - user which calls the cat command
#     file                    - full path name of file
#     { file_a "file_array" } - array name
#
#*******************************************************************************
proc get_file_content { host user file { file_a "file_array" } } {

   upvar $file_a back

   set program "cat"
   set program_arg $file
   set output [ start_remote_prog $host $user $program $program_arg]
   set lcounter 0
   if { $prg_exit_state != 0 } {
      add_proc_error "get_file_content" -1 "\'cat\' returned error: $output"
   } else {
      set help [ split $output "\n" ]
      foreach line $help {
         incr lcounter 1
         set back($lcounter) $line
      }
   }
   set back(0) $lcounter
}

#                                                             max. column:     |
#****** file_procedures/get_binary_path() ******
# 
#  NAME
#     get_binary_path -- get host specific binary path 
#
#  SYNOPSIS
#     get_binary_path { hostname binary } 
#
#  FUNCTION
#     This procedure will parse the binary-path.conf configuration file of the 
#     testsuite. In this file the user can configure his host specific binary 
#     path names. 
#
#  INPUTS
#     hostname - hostname where a binary should be found 
#     binary   - binary name (e.g. expect) 
#
#  RESULT
#     The full path name of the binary on the given host. The return value 
#     depends on the entries in the binary-path.conf file. 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     The binary-path.conf file has following syntax: 
#     Each line has 3  entries: 
#     hostname binary path. The $ARCH variable is resolved. 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     file_procedures/get_dir_names
#*******************************
proc get_binary_path { hostname binary } {
   global CHECK_OUTPUT CHECK_DEBUG_LEVEL
   global ts_host_config

   if { [ info exists ts_host_config($hostname,$binary) ] } {
      return $ts_host_config($hostname,$binary)
   } 

   add_proc_error "get_binary_path" -1 "no entry for binary \"$binary\" on host \"$hostname\" in host configuration"
   if { $CHECK_DEBUG_LEVEL >= 2 } {
      wait_for_enter
   }
   return $binary
}



#                                                             max. column:     |
#****** file_procedures/copy_directory() ******
# 
#  NAME
#     copy_directory -- copy a directory recursively 
#
#  SYNOPSIS
#     copy_directory { source target } 
#
#  FUNCTION
#     This procedure will copy the given source directory to the target 
#     directory. The content of the target dir is deleted if it exists. 
#     (calling delete_directory, which will make a secure copy in the testsuite 
#     trash folder). 
#
#  INPUTS
#     source - path to the source directory 
#     target - path to the target directory 
#
#  RESULT
#     no results 
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
#     file_procedures/delete_directory
#*******************************
proc copy_directory { source target } {

  if { ([string length $source] <= 10 ) || ([string length $target] <= 10 ) } {
     # just more security (do not create undefined dirs or something like that)
     add_proc_error "copy_directory" -1 "please use path with size > 10 characters"
     puts "please use path with size > 10 characters"
     return
  } 

  if { [ string compare $source $target ] == 0 } {
     add_proc_error "copy_directory" -1 "source and target are equal"
     puts "source and target are equal"
     return
  }
 
  set back [ catch { file mkdir $target } ]
  if { $back != 0 } {
     add_proc_error "copy_directory" -1 "can't create dir \"$target\""
     puts "can't create dir \"$target\""
     return
  }

  if { [file isdirectory $target] == 1 } {
      set back [ delete_directory $target ]
      if { $back != 0 } {
         add_proc_error "copy_directory" -1 "can't delete dir \"$target\""
         puts "can't delete dir \"$target\""
         return
      }
  }

  
  set back [ catch { file copy -- $source $target } ]
  if { $back != 0 } {
     add_proc_error "copy_directory" -1 "can't copy \"$source\" to \"$target\" "
     puts "can't create dir \"$target\""
     return
  }

  puts "no errors"

}


#                                                             max. column:     |
#****** file_procedures/cleanup_spool_dir() ******
# 
#  NAME
#     cleanup_spool_dir -- create or cleanup spool directory for master/execd 
#
#  SYNOPSIS
#     cleanup_spool_dir { topleveldir subdir } 
#
#  FUNCTION
#     This procedure will create or cleanup old entries in the qmaster or execd 
#     spool directory 
#
#  INPUTS
#     topleveldir - path to spool toplevel directory ( updir of qmaster and execd ) 
#     subdir      - this paramter is master or execd 
#
#  RESULT
#     if ok the procedure returns the correct spool directory. It returns  on 
#     error 
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
#     file_procedures/delete_directory
#*******************************
proc cleanup_spool_dir { topleveldir subdir } {
   global CHECK_COMMD_PORT CHECK_OUTPUT

   set spooldir "$topleveldir"

   puts $CHECK_OUTPUT "->spool toplevel directory is $spooldir"
   
   if { [ file isdirectory $spooldir ] == 1 } {
      set spooldir "$spooldir/$CHECK_COMMD_PORT"
      if { [ file isdirectory $spooldir ] != 1 } { 
          puts $CHECK_OUTPUT "creating directory \"$spooldir\""
          file mkdir $spooldir
          if { [ file isdirectory $spooldir ] != 1 } {
              puts $CHECK_OUTPUT "could not create directory \"$spooldir\""
              add_proc_error "cleanup_spool_dir" "-1" "could not create directory \"$spooldir\""
          }
      }
      set spooldir "$spooldir/$subdir"

      if { [ file isdirectory $spooldir ] != 1 } {
          puts $CHECK_OUTPUT "creating directory \"$spooldir\""
          file mkdir $spooldir
          if { [ file isdirectory $spooldir ] != 1 } {
              puts $CHECK_OUTPUT "could not create directory \"$spooldir\""
              add_proc_error "cleanup_spool_dir" "-1" "could not create directory \"$spooldir\""
          } 
      } else {
         if { [string compare $spooldir "" ] != 0 } {
             puts $CHECK_OUTPUT "deleting old spool dir entries in \"$spooldir\""
             if { [delete_directory $spooldir] != 0 } { 
                puts $CHECK_OUTPUT "could not remove spool directory $spooldir"
                add_proc_error "cleanup_spool_dir" -2 "could not remove spool directory $spooldir"
             }
             puts $CHECK_OUTPUT "creating directory \"$spooldir\""
             file mkdir $spooldir
             if { [ file isdirectory $spooldir ] != 1 } {
                puts $CHECK_OUTPUT "could not create directory \"$spooldir\" after moving to trash folder"
                add_proc_error "cleanup_spool_dir" "-1" "could not create directory \"$spooldir\""
             } 
         }
      }
      
      puts $CHECK_OUTPUT "local spooldir is \"$spooldir\""
   } else {
      add_proc_error "cleanup_spool_dir" "-1" "toplevel spool directory \"$spooldir\" not found"
      puts $CHECK_OUTPUT "using no spool dir"
      set spooldir ""
   }
   puts "$spooldir"
   return $spooldir
}



#                                                             max. column:     |
#****** file_procedures/delete_file_at_startup() ******
# 
#  NAME
#     delete_file_at_startup -- delete old temp files 
#
#  SYNOPSIS
#     delete_file_at_startup { filename } 
#
#  FUNCTION
#     This procedure will delete every file added to the file 
#     $CHECK_TESTSUITE_ROOT/.testsuite_delete on the startup of a testrun 
#
#  INPUTS
#     filename - full path file name of file to add to 
#                $CHECK_TESTSOUTE_ROOT/.testsuite_delete file 
#
#  RESULT
#     no results 
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
#     file_procedures/delete_directory
#*******************************
proc delete_file_at_startup { filename } {
   global CHECK_OUTPUT CHECK_TESTSUITE_ROOT

   set del_file_name "$CHECK_TESTSUITE_ROOT/.testsuite_delete"
   if {[file isfile $del_file_name] != 1} {
       set del_file [ open $del_file_name "w" ]
   } else {
       set del_file [ open $del_file_name "a" ]
   }
   puts $del_file "$filename"
   close $del_file    
}


#                                                             max. column:     |
#****** file_procedures/delete_file() ******
# 
#  NAME
#     delete_file -- move/copy file to testsuite trashfolder 
#
#  SYNOPSIS
#     delete_file { filename } 
#
#  FUNCTION
#     This procedure will move/copy the file to the testsuite's trashfolder 
#     (Directory testsuite_trash in the testsuite root directory). 
#
#  INPUTS
#     filename - full path file name of file 
#
#  RESULT
#     no results 
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
#     file_procedures/delete_directory
#*******************************
proc delete_file { filename } { 
 
   global CHECK_OUTPUT CHECK_TESTSUITE_ROOT

   wait_for_file "$filename" 200 0 0 ;# wait for file, no error reporting!

   if {[file isdirectory "$CHECK_TESTSUITE_ROOT/testsuite_trash"] != 1} {
      file mkdir "$CHECK_TESTSUITE_ROOT/testsuite_trash"
   }

   if {[file isfile "$filename"] != 1} {
      puts $CHECK_OUTPUT "delete_file - no such file: \"$filename\""
      add_proc_error "delete_file" "-1" "no such file: \"$filename\""
      return      
   }

   set deleted_file 0 
   if { [string length $filename ] > 10 } {
      debug_puts "delete_file - moving \"$filename\" to trash folder ..."
      set new_name [ file tail $filename ] 
      set catch_return [ catch { 
         eval exec "mv $filename $CHECK_TESTSUITE_ROOT/testsuite_trash/$new_name.[timestamp]" 
      } result ] 
      if { $catch_return != 0 } {
         puts $CHECK_OUTPUT "delete_file - mv error:\n$result"
         puts $CHECK_OUTPUT "delete_file - try to copy the file"
         set catch_return [ catch { 
            eval exec "cp $filename $CHECK_TESTSUITE_ROOT/testsuite_trash/$new_name.[timestamp]" 
         } result ] 
         if { $catch_return != 0 } {
           puts $CHECK_OUTPUT "could not mv/cp file \"$filename\" to trash folder"
           add_proc_error "delete_file" "-1" "could not mv/cp file \"$filename\" to trash folder"
         } else {
           puts $CHECK_OUTPUT "copy ok - deleting file \"$filename\""
           set catch_return [ catch { eval exec "rm $filename" } result ] 
           if { $catch_return != 0 } {
              puts $CHECK_OUTPUT "could not remove file \"$filename\""
              puts $CHECK_OUTPUT "$result"
              add_proc_error "delete_file" "-1" "could not remove file \"$filename\" - $result"
           } else {
              puts $CHECK_OUTPUT "done"
              set deleted_file 1
           }
         }
      } else {
        set deleted_file 1
      }
      if { $deleted_file == 1 } {
         wait_for_file "$filename" "200" "1" ;# wait for file do disappear in filesystem!
      }
   } else {
      puts $CHECK_OUTPUT "delete_file - file path is to short. Will not delete\n\"$filename\""
      add_proc_error "delete_file" "-1" "file path is to short. Will not delete\n\"$filename\""
   }
}


#                                                             max. column:     |
#****** file_procedures/wait_for_file() ******
# 
#  NAME
#     wait_for_file -- wait for file to appear/dissappear/... 
#
#  SYNOPSIS
#     wait_for_file { path_to_file seconds { to_go_away 0 } 
#     { do_error_check 1 } } 
#
#  FUNCTION
#     Wait a given number of seconds fot the creation or deletion of a file. 
#
#  INPUTS
#     path_to_file         - full path file name of file 
#     seconds              - timeout in seconds 
#     { to_go_away 0 }     - flag, (0=wait for creation, 1 wait for deletion) 
#     { do_error_check 1 } - flag, (0=do not report errors, 1 report errors) 
#
#  RESULT
#     -1 for an unsuccessful waiting, 0 no errors 
#
#  SEE ALSO
#     file_procedures/delete_directory
#     sge_procedures/wait_for_load_from_all_queues
#     file_procedures/wait_for_file
#     sge_procedures/wait_for_jobstart
#     sge_procedures/wait_for_end_of_transfer
#     sge_procedures/wait_for_jobpending
#     sge_procedures/wait_for_jobend
#*******************************
proc wait_for_file { path_to_file seconds { to_go_away 0 } { do_error_check 1 } } {
   global CHECK_OUTPUT 
   
   debug_puts "Looking for file $path_to_file."
   set time [ expr ( [timestamp] + $seconds  ) ]
   set wasok -1
   
   if { $to_go_away == 0 } {
      puts $CHECK_OUTPUT "Looking for creation of the file \"$path_to_file\" ..."
      while { [timestamp] < $time }  {
        if { [ file isfile "$path_to_file"] } {
           set wasok 0
           break
        }
        sleep 1
      }
      if { ($wasok != 0) && ($do_error_check == 1) } {
         add_proc_error "wait_for_file" -1 "timeout error while waiting for creation of file \"$path_to_file\""
      } 
   } else {
      puts $CHECK_OUTPUT "Looking for deletion of the file \"$path_to_file\" ..."
      while { [timestamp] < $time }  {
        if { [ file isfile "$path_to_file"] != 1 } {
           set wasok 0
           break
        }
        sleep 1
      }
      if {($wasok != 0) && ($do_error_check == 1) } {
         add_proc_error "wait_for_file" -1 "timeout error while waiting for deletion file \"$path_to_file\""
      } 
   }
   return $wasok
}


#****** file_procedures/wait_for_remote_file() *********************************
#  NAME
#     wait_for_remote_file() -- waiting for a file to apear (NFS-Check)
#
#  SYNOPSIS
#     wait_for_remote_file { hostname user path { mytimeout 60 } } 
#
#  FUNCTION
#     The function is using the ls command on the remote host. If the command
#     returns no error the procedure returns. Otherwise an error is reported
#     when reaching timeout value.
#
#  INPUTS
#     hostname         - host where the file should be checked
#     user             - user id who performs check
#     path             - full path to file
#     { mytimeout 60 } - timeout in seconds
#
#  SEE ALSO
#     file_procedures/wait_for_file()
#*******************************************************************************
proc wait_for_remote_file { hostname user path { mytimeout 60 } } {
   global CHECK_OUTPUT

   set is_ok 0
   set my_mytimeout [ expr ( [timestamp] + $mytimeout ) ] 

   while { $is_ok == 0 } {
      set output [ start_remote_prog $hostname $user "ls" "$path" prg_exit_state 60 0 "" 0]
      if { $prg_exit_state == 0 } {
         set is_ok 1
         break
      } 
      puts -nonewline $CHECK_OUTPUT "."
      if { [timestamp] > $my_mytimeout } {
         break
      }
      sleep 1
   }
   if { $is_ok == 1 } {
      puts $CHECK_OUTPUT "ok"
      puts $CHECK_OUTPUT "found prog: $output"
   } else {
      puts $CHECK_OUTPUT "timeout"
      add_proc_error "wait_for_remote_file" -1 "timeout while waiting for file $path on host $hostname"
   }
}



#                                                             max. column:     |
#****** file_procedures/delete_directory() ******
# 
#  NAME
#     delete_directory -- move/copy directory to testsuite trashfolder 
#
#  SYNOPSIS
#     delete_directory { path } 
#
#  FUNCTION
#     This procedure will move/copy the given directory to the testsuite's 
#     trashfolder (Directory testsuite_trash in the testsuite root directory). 
#
#  INPUTS
#     path - full directory path 
#
#  RESULT
#     -1 on error, 0 ok 
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
#     file_procedures/delete_directory
#*******************************

proc delete_directory { path } { 
   global CHECK_OUTPUT CHECK_TESTSUITE_ROOT

   set return_value -1
   puts $CHECK_OUTPUT "path to delete: \"$path\""
   if {[file isdirectory "$CHECK_TESTSUITE_ROOT/testsuite_trash"] != 1} {
      file mkdir "$CHECK_TESTSUITE_ROOT/testsuite_trash"
   }

   if {[file isdirectory "$path"] != 1} {
      puts $CHECK_OUTPUT "delete_directory - no such directory: \"$path\""
      add_proc_error "delete_directory" -1 "no such directory: \"$path\""
      return -1     
   }
 
   if { [string length $path ] > 10 } {
      puts $CHECK_OUTPUT "delete_directory - moving \"$path\" to trash folder ..."
      set new_name [ file tail $path ] 
      set catch_return [ catch { 
         eval exec "mv $path $CHECK_TESTSUITE_ROOT/testsuite_trash/$new_name.[timestamp]" 
      } result ] 
      if { $catch_return != 0 } {
         puts $CHECK_OUTPUT "delete_directory - mv error:\n$result"
         puts $CHECK_OUTPUT "delete_directory - try to copy the directory"
         set catch_return [ catch { 
            eval exec "cp -r $path $CHECK_TESTSUITE_ROOT/testsuite_trash/$new_name.[timestamp]" 
         } result ] 
         if { $catch_return != 0 } {
            puts $CHECK_OUTPUT "could not mv/cp directory \"$path\" to trash folder, $result"
            add_proc_error "delete_directory" -1 "could not mv/cp directory \"$path\" to trash folder, $result"
            set return_value -1
         } else { 
            puts $CHECK_OUTPUT "copy ok -  removing directory"
            set catch_return [ catch { 
               eval exec "rm -rf $path" 
            } result ] 
            if { $catch_return != 0 } {
               puts $CHECK_OUTPUT "could not remove directory \"$path\", $result"
               add_proc_error "delete_directory" -1 "could not remove directory \"$path\", $result"
               set return_value -1
            } else {
               puts $CHECK_OUTPUT "done"
               set return_value 0
            }
         }
      } else {
         set return_value 0
      }
   } else {
      puts $CHECK_OUTPUT "delete_directory - path is to short. Will not delete\n\"$path\""
      add_proc_error "delete_directory" "-1" "path is to short. Will not delete\n\"$path\""
      set return_value -1
   }
  return $return_value
}

#****** file_procedures/init_logfile_wait() ************************************
#  NAME
#     init_logfile_wait() -- observe logfiles by using tail functionality (1)
#
#  SYNOPSIS
#     init_logfile_wait { hostname logfile } 
#
#  FUNCTION
#     This procedure is using the reserved open remote spawn connection
#     "ts_def_con" in order to start a tail process that observes the given
#     file. The open spawn id is stored in a global variable to make it
#     possible for the logfile_wait() procedure to expect data from
#     the tail process.
#     Each call of this procedure must follow a call of logfile_wait() in
#     order to close the open spawn process.
#
#  INPUTS
#     hostname - host where tail should be started
#     logfile  - full path name of (log)file
#
#  SEE ALSO
#     file_procedures/logfile_wait()
#     file_procedures/close_logfile_wait()
#*******************************************************************************
proc init_logfile_wait { hostname logfile  } {

   global CHECK_OUTPUT
   global file_procedure_logfile_wait_sp_id

   set sid [ open_remote_spawn_process $hostname "ts_def_con" "tail" "-f $logfile"]
   set sp_id [lindex $sid 1]
   set timeout 5
   puts $CHECK_OUTPUT "spawn id: $sp_id"

   log_user 0
   while { 1 } {
      expect {
         -i $sp_id -- full_buffer {
            add_proc_error "init_logfile_wait" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
            break
         }

         -i $sp_id eof {
            break
         }
         -i $sp_id -- "_exit_status_" {
            break
         }
         -i $sp_id timeout {
            break
         }
         -i $sp_id -- "\n" {
            puts -nonewline $CHECK_OUTPUT $expect_out(buffer)
         }
         -i $sp_id default {
            break
         }
      }
   }
   log_user 1 
   puts $CHECK_OUTPUT "init_logfile_wait done"
   set file_procedure_logfile_wait_sp_id $sid
}

#****** file_procedures/logfile_wait() *****************************************
#  NAME
#     logfile_wait() -- observe logfiles by using tail functionality (2)
#
#  SYNOPSIS
#     logfile_wait {
#                    { wait_string ""     } 
#                    { mytimeout 60       }
#                    { close_connection 1 } 
#                    { add_errors 1       } 
#                    { return_err_code "logfile_wait_error" }
#                  } 
#
#  FUNCTION
#     This procedure is called after an init_logfile_wait() call. It will
#     use the open spawn process started from that procedure. When the
#     output of the tail command contains the string given in "wait_string"
#     the procedure returns immediately. If the caller hasn't provided an
#     "wait_string" the procedure returns after the given timeout without
#     error.
#
#  INPUTS
#     { wait_string "" }     - if the tail process generates output 
#                              containing this string the procedure 
#                              returns
#     { mytimeout 60 }       - timeout in seconds
#
#     { close_connection 1 } - if 0, don't close tail process
#
#     { add_errors 1       } - if 0, don't call add_proc_error()
#
#     { return_err_code "logfile_wait_error" } 
#                            - variable where the return
#                              value is stored:
#                              0  : no error
#                              -1 : timeout error
#                              -2 : full expect buffer 
#                              -3 : unexpected end of file
#                              -4 : unexpected end of tail command
#
#  RESULT
#     This procedure returns the output of the tail command since the 
#     init_logfile_wait() call.
# 
#  SEE ALSO
#     file_procedures/init_logfile_wait()
#     file_procedures/close_logfile_wait()
#*******************************************************************************
proc logfile_wait { { wait_string "" } { mytimeout 60 } { close_connection 1 } { add_errors 1 } { return_err_code "logfile_wait_error" } } {
   global file_procedure_logfile_wait_sp_id
   global CHECK_OUTPUT

   upvar $return_err_code back_value


   set back_value 0

   set sp_id [ lindex $file_procedure_logfile_wait_sp_id 1 ]
   puts $CHECK_OUTPUT "spawn id: $sp_id"
   set real_timeout [ expr ( [timestamp] + $mytimeout  )  ]
   set timeout 1
   set my_tail_buffer ""
   log_user 0
   while { 1 } {
      if { [timestamp] > $real_timeout } {
          if { $wait_string != "" } {
             if { $add_errors == 1 } {
                add_proc_error "logfile_wait" -1 "timeout waiting for logfile content"
             }
             set back_value -1
          }
          break
      }
      expect {
         -i $sp_id -- full_buffer {
            if { $add_errors == 1 } {
               add_proc_error "init_logfile_wait" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
            }
            set back_value -2
            break
         }

         -i $sp_id eof {
            if { $add_errors == 1 } {
               add_proc_error "init_logfile_wait" "-1" "unexpected end of file"
            }
            set back_value -3
            break
         }
         -i $sp_id -- "_exit_status_" { 
            if { $add_errors == 1 } {
               add_proc_error "init_logfile_wait" "-1" "unexpected end of tail command"
            }
            set back_value -4
            break
         }
         -i $sp_id timeout {
            puts -nonewline $CHECK_OUTPUT [washing_machine [ expr ( $real_timeout - [ timestamp ] + 1  ) ]]
            flush $CHECK_OUTPUT
         }
         -i $sp_id -- "\n" {
            puts -nonewline $CHECK_OUTPUT "\r$expect_out(buffer)"
            append my_tail_buffer $expect_out(buffer)
            if { $wait_string != "" } {
               if { [ string first $wait_string $expect_out(buffer)] >= 0  } {
                  break;
               }
            }
         }

         -i $sp_id default {
            break
         }

      }
   }
   puts $CHECK_OUTPUT ""
   if { $close_connection == 1 } {
      uplevel 1 { close_spawn_process $file_procedure_logfile_wait_sp_id }
   }
   log_user 1
   return $my_tail_buffer
}

#****** file_procedures/close_logfile_wait() ***********************************
#  NAME
#     close_logfile_wait() -- close open_spawn_connection id for tail process
#
#  SYNOPSIS
#     close_logfile_wait { } 
#
#  FUNCTION
#     This procedure is used for closing an open tail process, started with
#     init_logfile_wait(), when logfile_wait() is called with 
#     "close_connection != 1" parameter.
#
#  SEE ALSO
#     file_procedures/init_logfile_wait()
#     file_procedures/logfile_wait()
#*******************************************************************************
proc close_logfile_wait { } {
   global file_procedure_logfile_wait_sp_id
   global CHECK_OUTPUT

   uplevel 1 { close_spawn_process $file_procedure_logfile_wait_sp_id }
}

#****** file_procedures/washing_machine() **************************************
#  NAME
#     washing_machine() -- showing a washing machine ;-)
#
#  SYNOPSIS
#     washing_machine { time { small 0 } } 
#
#  FUNCTION
#     This procedure returns "\r[/|\-] $time [/|\-]", depending on the 
#     given time value.
#
#  INPUTS
#     time      - timout counter 
#     { small } - if > 0 -> just return [/|\-], depending on $time
#
#  RESULT
#     string, e.g. "/ 40 /"
#*******************************************************************************
proc washing_machine { time { small 0 } } {
   set ani [ expr ( $time % 4 ) ]
   switch $ani {
      0 { set output "-" }
      1 { set output "/" }
      2 { set output "|" }
      3 { set output "\\" }
   }
   if { $small != 0 } {
      return "$output"
   } else {
      return "\r              \r$output $time $output\r"
   }
}

#****** file_procedures/create_path_aliasing_file() ****************************
#  NAME
#     create_path_aliasing_file() -- ??? 
#
#  SYNOPSIS
#     create_path_aliasing_file { filename data elements } 
#
#  FUNCTION
#     This procedure will create a path aliasing file.
#
#  INPUTS
#     filename - full path file name of path aliasing file
#     data     - data array with following fields:
#                arrayname(src-path,$i)
#                arrayname(sub-host,$i)
#                arrayname(exec-host,$i)
#                arrayname(replacement,$i)
#                where $i is the index number of each entry 
#     elements - nr. of entries (starting from zero)
#
#  EXAMPLE
#     set data(src-path,0)     "/tmp_mnt/"
#     set data(sub-host,0)     "*"
#     set data(exec-host,0)    "*" 
#     set data(replacement,0)  "/home/"
#     create_path_aliasing_file /tmp/test.txt data 1
#      
#  SEE ALSO
#     file_procedures/create_shell_script()
#     
#    
#*******************************************************************************
proc create_path_aliasing_file { filename data elements} {
   global CHECK_OUTPUT
 
   upvar $data mydata 
    
# Path Aliasing File
# src-path   sub-host   exec-host   replacement
#     /tmp_mnt/    *          *           /
# replaces any occurrence of /tmp_mnt/ by /
# if submitting or executing on any host.
# Thus paths on nfs server and clients are the same

#     <sge_root>/<cell>/common/sge_aliases    global alias file
#     $HOME/.sge_aliases                         user local aliases file

   if { [ file isfile $filename ] == 1 } {
      add_proc_error "create_path_aliasing_file" -1 "file $filename already exists"
      return
   }

   puts $CHECK_OUTPUT "creating path alias file: $filename"
   set fout [ open "$filename" "w" ] 
   puts $fout "# testsuite automatic generated Path Aliasing File\n# \"$filename\""
   puts $fout "# src-path   sub-host   exec-host   replacement"
   puts $fout "#     /tmp_mnt/    *          *           /"
   puts $fout "# replaces any occurrence of /tmp_mnt/ by /"
   puts $fout "# if submitting or executing on any host."
   puts $fout "# Thus paths on nfs server and clients are the same"
   puts $fout "##########"
   puts $fout "# <sge_root>/<cell>/common/sge_aliases    global alias file"
   puts $fout "# \$HOME/.sge_aliases                         user local aliases file"
   puts $fout "##########"
   puts $fout "# src-path   sub-host   exec-host   replacement"
   for { set i 0} { $i < $elements} { incr i 1 } {
       if { [info exists mydata(src-path,$i) ] != 1 } {
          add_proc_error "create_path_aliasing_file" -1 "array has no (src-path,$i) element"
          break
       } 
       set    line "[ set mydata(src-path,$i) ]\t"
       append line "[ set mydata(sub-host,$i) ]\t" 
       append line "[ set mydata(exec-host,$i) ]\t"
       append line "[ set mydata(replacement,$i) ]"
       puts $fout $line
   } 
   flush $fout
   close $fout
   puts $CHECK_OUTPUT "closing file"
}


if { [info exists argc ] != 0 } {
   set TS_ROOT ""
   set procedure ""
   for { set i 0 } { $i < $argc } { incr i } {
      if {$i == 0} { set TS_ROOT [lindex $argv $i] }
      if {$i == 1} { set procedure [lindex $argv $i] }
   }
   if { $argc == 0 } {
      puts "usage:\nfile_procedures.tcl <CHECK_TESTSUITE_ROOT> <proc> no_main <testsuite params>"
      puts "options:"
      puts "CHECK_TESTSUITE_ROOT -  path to TESTSUITE directory"
      puts "proc                 -  procedure from this file with parameters"
      puts "no_main              -  used to source testsuite file (check.exp)"
      puts "testsuite params     -  any testsuite command option (from file check.exp)"
      puts "                        testsuite params: file <path>/defaults.sav is needed"
   } else {
      source "$TS_ROOT/check.exp"
      if { $be_quiet == 0 } {
          puts $CHECK_OUTPUT "master host is $CHECK_CORE_MASTER"
          puts $CHECK_OUTPUT "calling \"$procedure\" ..."
      }
      set result [ eval $procedure ]
      puts $result 
      flush $CHECK_OUTPUT
   }
} 


