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


#                                                             max. column:     |
#****** file_procedures/create_shell_script() ******
# 
#  NAME
#     create_shell_script -- create a /bin/sh script file 
#
#  SYNOPSIS
#     create_shell_script { scriptfile exec_command exec_arguments } 
#
#  FUNCTION
#     This procedure generates a script which will execute the given command. 
#     The script will restore the testsuite and SGE environment first. It will 
#     also echo _start_mark_:(x) and _exit_status_:(x) where x is the exit 
#     value from the started command. 
#
#  INPUTS
#     scriptfile     - full path and name of scriptfile to generate 
#     exec_command   - command to execute 
#     exec_arguments - command parameters 
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
#     file_procedures/get_dir_names
#*******************************
proc create_shell_script { scriptfile exec_command exec_arguments } {
   global CHECK_OUTPUT CHECK_PRODUCT_TYPE CHECK_COMMD_PORT CHECK_PRODUCT_ROOT
   global CHECK_DEBUG_LEVEL 


   set script [ open "$scriptfile" "w" ]


   # script header
   puts $script "#!/bin/sh"
   puts $script "# Automatic generated script from Grid Engine Testsuite"
   puts $script "# The script will execute a special command with arguments"
   puts $script "# and it should be deleted after use. So if this file exists, please delete it"

   # script command
   puts $script "trap 'echo \"_exit_status_:(1)\"' 0"
   puts $script "umask 022"
   puts $script "if \[ -f $CHECK_PRODUCT_ROOT/default/common/settings.sh \]; then"

   puts $script "   . $CHECK_PRODUCT_ROOT/default/common/settings.sh"
   puts $script "else"
   puts $script "   COMMD_PORT=$CHECK_COMMD_PORT"
   puts $script "   SGE_ROOT=$CHECK_PRODUCT_ROOT"
   puts $script "   export COMMD_PORT"
   puts $script "   export SGE_ROOT"
   puts $script "fi"
   puts $script "echo \"_start_mark_:(\$?)\""
   puts $script "$exec_command $exec_arguments"
   puts $script "exit_val=\"\$?\""
   puts $script "trap 0"
   puts $script "echo \"_exit_status_:(\$exit_val)\""
   puts $script "echo \"script done.\""
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
   global CHECK_CONFIG_DIR CHECK_BINARY_DIR_CONFIG_FILE
   global CHECK_OUTPUT CHECK_DEBUG_LEVEL

   set config "$CHECK_CONFIG_DIR/$CHECK_BINARY_DIR_CONFIG_FILE"  
   set binary_path ""


   if { [file exists $config] } {
      set file_p [ open $config r ]
      set line_no 0
      while { [gets $file_p line] >= 0 } {
         if { [string first "#" $line ] == 0 } {
            debug_puts "found comment in line $line_no"
            incr line_no 1
            continue
         }
         set tmp_host [ lindex $line 0 ]
         set tmp_bin  [ lindex $line 1 ]
         set tmp_path [ lindex $line 2 ]
         if { [ string compare $tmp_bin $binary] == 0 } {
            debug_puts "found matching binary entry $tmp_bin"
            debug_puts "LINE: $line"
            set arch_start -1
            set arch_end -1
            set arch_start [ string first "\$ARCH" $tmp_path ] 
            set arch_start [ expr ( $arch_start - 1 ) ]
            set arch_end   [ expr ( $arch_start + 6 ) ]
            if { $arch_start >= 0 } {
               debug_puts "found ARCH variable match"
               if { [string compare [resolve_arch $hostname] "unknown" ] == 0 } {
                  add_proc_error "get_binary_path" -1 "can not resolve ARCH variable, please enter full pathname"
                  incr line_no 1
                  continue
               }
               set new_path "[ string range $tmp_path 0 $arch_start ][resolve_arch $hostname][string range $tmp_path $arch_end end]"
               debug_puts "new path: $new_path"
               set tmp_path $new_path
            }

            if { [ string compare $binary_path "" ] == 0 && [ string compare $tmp_host "all"] == 0 } {
               debug_puts "found global entry for all hosts"
               set binary_path $tmp_path
            }
            debug_puts "$tmp_host, $hostname" 
            if { [ string compare $tmp_host $hostname ] == 0 } {
               debug_puts "found entry for host $hostname"
               set binary_path $tmp_path
            }
         }
         incr line_no 1
      }
      close $file_p 
   } else {
     add_proc_error "get_binary_path" -1 "config file \"$config\" not found"
     set binary_path $binary
   }

   if { [ string compare $binary_path "" ] == 0 } {
     add_proc_error "get_binary_path" -1 "config file \"$config\" has no entry for binary \"$binary\" on host \"$hostname\"."
     set binary_path $binary
   } else {
      if { [file exists $binary_path] != 1 } {
        add_proc_error "get_binary_path" -1 "binary file \"$binary_path\" not found (config file is \"$config\")"
        set binary_path $binary
      }
   }

   debug_puts "get_binary_path: $binary on host $hostname is: $binary_path"

   if { $CHECK_DEBUG_LEVEL == 2 } {
      wait_for_enter
   }

   return $binary_path
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

   if { [ file isfile $filename ] != 1 } {
      return 
   }
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

