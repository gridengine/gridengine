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


global ts_host_config               ;# new testsuite host configuration array
global actual_ts_host_config_version      ;# actual host config version number
set    actual_ts_host_config_version "1.2"

if {![info exists ts_host_config]} {
   # ts_host_config defaults
   set parameter "version"
   set ts_host_config($parameter)            "$actual_ts_host_config_version"
   set ts_host_config($parameter,desc)       "Testuite host configuration setup"
   set ts_host_config($parameter,default)    "$actual_ts_host_config_version"
   set ts_host_config($parameter,setup_func) ""
   set ts_host_config($parameter,onchange)   "stop"
   set ts_host_config($parameter,pos)        1

   set parameter "hostlist"
   set ts_host_config($parameter)            ""
   set ts_host_config($parameter,desc)       "Testsuite cluster host list"
   set ts_host_config($parameter,default)    ""
   set ts_host_config($parameter,setup_func) "host_config_$parameter"
   set ts_host_config($parameter,onchange)   "install"
   set ts_host_config($parameter,pos)        2
}

#****** config/host/host_config_hostlist() *******************************************
#  NAME
#     host_config_hostlist() -- host configuration setup
#
#  SYNOPSIS
#     host_config_hostlist { only_check name config_array } 
#
#  FUNCTION
#     Testsuite host configuration setup - called from verify_host_config()
#
#  INPUTS
#     only_check   - 0: expect user input
#                    1: just verify user input
#     name         - option name (in ts_host_config array)
#     config_array - config array name (ts_config)
#
#  SEE ALSO
#     check/setup_host_config()
#     check/verify_host_config()
#*******************************************************************************
proc host_config_hostlist { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER

   upvar $config_array config

   set description   $config($name,desc)

   if { $only_check == 0 } {
       set not_ready 1
       while { $not_ready } {
          clear_screen
          puts $CHECK_OUTPUT "----------------------------------------------------------"
          puts $CHECK_OUTPUT "Global host configuration setup"
          puts $CHECK_OUTPUT "----------------------------------------------------------"
          puts $CHECK_OUTPUT "\n    hosts configured: [llength $config(hostlist)]"
          host_config_hostlist_show_hosts config
          puts $CHECK_OUTPUT "\n\n(1)  add host"
          puts $CHECK_OUTPUT "(2)  edit host"
          puts $CHECK_OUTPUT "(3)  delete host"
          puts $CHECK_OUTPUT "(4)  try nslookup scann"
          puts $CHECK_OUTPUT "(10) exit setup"
          puts -nonewline $CHECK_OUTPUT "> "
          set input [ wait_for_enter 1]
          switch -- $input {
             1 {
                set result [host_config_hostlist_add_host config]
                if { $result != 0 } {
                   wait_for_enter
                }
             }
             2 {
                set result [host_config_hostlist_edit_host config]
                if { $result != 0 } {
                   wait_for_enter
                }
             }
             3 {
               set result [host_config_hostlist_delete_host config]
                if { $result != 0 } {
                   wait_for_enter
                }
             }
             10 {
                set not_ready 0
             }
             4 {
                set result [ start_remote_prog $CHECK_HOST $CHECK_USER "nslookup" $CHECK_HOST prg_exit_state 60 0 "" 1 0 ]
                if { $prg_exit_state == 0 } {
                   set pos1 [ string first $CHECK_HOST $result ]
                   set ip [string range $result $pos1 end]
                   set pos1 [ string first ":" $ip ]
                   incr pos1 1
                   set ip [string range $ip $pos1 end]
                   set pos1 [ string last "." $ip ]
                   incr pos1 -1
                   set ip [string range $ip 0 $pos1 ]
                   set ip [string trim $ip]
                   puts $CHECK_OUTPUT "ip: $ip"

                   for { set i 1 } { $i <= 254 } { incr i 1 } {
                       set ip_run "$ip.$i"
                       puts -nonewline $CHECK_OUTPUT "\r$ip_run"
                       set result [ start_remote_prog $CHECK_HOST $CHECK_USER "nslookup" $ip_run prg_exit_state 25 0 "" 1 0 ]
                       set pos1 [ string first "Name:" $result ]   
                       if { $pos1 >= 0 } {
                          incr pos1 5
                          set name [ string range $result $pos1 end ]
                          set pos1 [ string first "." $name ]
                          incr pos1 -1
                          set name [ string range $name 0 $pos1 ]
                          set name [ string trim $name ]
                          puts $CHECK_OUTPUT "\nHost: $name"
                          set result [host_config_hostlist_add_host config $name]
                       }
                   }
                } 

                wait_for_enter
             }
          } 
       }
   } 

   # check host configuration
   debug_puts "host_config_hostlist:"
   foreach host $config(hostlist) {
      debug_puts "      host: $host"
   }

   return $config(hostlist)
}


#****** config/host/host_config_hostlist_show_hosts() ********************************
#  NAME
#     host_config_hostlist_show_hosts() -- show host in host configuration
#
#  SYNOPSIS
#     host_config_hostlist_show_hosts { array_name } 
#
#  FUNCTION
#     Print hosts
#
#  INPUTS
#     array_name - ts_host_config
#
#  SEE ALSO
#     check/setup_host_config()
#     check/verify_host_config()
#     check/host_config_hostlist_show_compile_hosts()
#*******************************************************************************
proc host_config_hostlist_show_hosts { array_name } {
   global CHECK_OUTPUT
   upvar $array_name config

   puts $CHECK_OUTPUT "\nHost list:\n"
   if { [llength $config(hostlist)] == 0 } {
      puts $CHECK_OUTPUT "no hosts defined"
   }

   set max_length 0
   foreach host $config(hostlist) {
      if { [string length $host] > $max_length } {
         set max_length [string length $host]
      }
   }  


   set index 0
   foreach host $config(hostlist) {
      incr index 1 

      set space ""
      for { set i 0 } { $i < [ expr ( $max_length - [ string length $host ]  ) ] } { incr i 1 } {  
          append space " "
      }
      if { $config($host,compile) == 1 } {
          set comp_host "(compile host)"
      } else {
          set comp_host "              "
      }


      if { $index <= 9 } {
         puts $CHECK_OUTPUT "    $index) $host $space ($config($host,arch)) $comp_host"
      } else {
         puts $CHECK_OUTPUT "   $index) $host $space ($config($host,arch)) $comp_host"
      }
   }
}

#****** config/host/host_config_hostlist_show_compile_hosts() ************************
#  NAME
#     host_config_hostlist_show_compile_hosts() -- show compile hosts
#
#  SYNOPSIS
#     host_config_hostlist_show_compile_hosts { array_name save_array } 
#
#  FUNCTION
#     This procedure shows the list of compile hosts
#
#  INPUTS
#     array_name - ts_host_config array
#     save_array - array to store compile host informations
#
#  RESULT
#     save_array(count)     -> number of compile hosts (starting from 1)
#     save_array($num,arch) -> compile architecture
#     save-array($num,host) -> compile host name
#
#  SEE ALSO
#     check/host_config_hostlist_show_hosts()
#*******************************************************************************
proc host_config_hostlist_show_compile_hosts { array_name save_array } {
   global CHECK_OUTPUT
   upvar $array_name config
   upvar $save_array back

   if { [ info exists back ] } {
      unset back
   }

   puts $CHECK_OUTPUT "\nCompile architecture list:\n"
   if { [llength $config(hostlist)] == 0 } {
      puts $CHECK_OUTPUT "no hosts defined"
   }

   set index 0
  
   set max_arch_length 0
   foreach host $config(hostlist) {
      if { $config($host,compile) == 1 } {
         lappend arch_list $config($host,arch)
         set host_list($config($host,arch)) $host
         set arch_length [string length $config($host,arch) ]
         if { $max_arch_length < $arch_length } {
            set max_arch_length $arch_length
         }
      }
   }
   set arch_list [lsort $arch_list]
   foreach arch $arch_list {
      set host $host_list($arch)
      if { $config($host,compile) == 1 } {
         incr index 1 
         set back(count) $index
         set back($index,arch) $config($host,arch)
         set back($index,host) $host 

         set arch_length [string length $config($host,arch)]
         if { $index <= 9 } {
            puts $CHECK_OUTPUT "    $index) $config($host,arch) [get_spaces [expr ( $max_arch_length - $arch_length ) ]] ($host)"
         } else {
            puts $CHECK_OUTPUT "   $index) $config($host,arch) [get_spaces [expr ( $max_arch_length - $arch_length ) ]] ($host)" 
         }
      } 
   }
}



#****** config/host/host_config_hostlist_add_host() **********************************
#  NAME
#     host_config_hostlist_add_host() -- add host to host configuration
#
#  SYNOPSIS
#     host_config_hostlist_add_host { array_name { have_host "" } } 
#
#  FUNCTION
#     This procedure is used to add an host to the testsuite host configuration 
#
#  INPUTS
#     array_name       - ts_host_config
#     { have_host "" } - if not "": add this host without questions
#
#  SEE ALSO
#     check/setup_host_config()
#     check/verify_host_config()
#*******************************************************************************
proc host_config_hostlist_add_host { array_name { have_host "" } } {
   global CHECK_OUTPUT
   upvar $array_name config
   global CHECK_USER
  
   if { $have_host == "" } {
      clear_screen
      puts $CHECK_OUTPUT "\nAdd host to global host configuration"
      puts $CHECK_OUTPUT "====================================="

   
      host_config_hostlist_show_hosts config

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter new hostname: "
      set new_host [wait_for_enter 1]
   } else {
      set new_host $have_host
   }

   if { [ string length $new_host ] == 0 } {
      puts $CHECK_OUTPUT "no hostname entered"
      return -1
   }
     
   if { [ lsearch $config(hostlist) $new_host ] >= 0 } {
      puts $CHECK_OUTPUT "host \"$new_host\" is allready in list"
      return -1
   }

   set time [timestamp]
   set result [ start_remote_prog $new_host $CHECK_USER "echo" "\"hello $new_host\"" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      if { $have_host == "" } {

         puts $CHECK_OUTPUT "connect timeout error\nPlease enter a timeout value > 12 or press return to abort"
         set result [ wait_for_enter 1 ]
         if { [ string length $result] == 0  || $result < 12 } {
            puts $CHECK_OUTPUT "aborting ..."
            return -1
         }
         set result [ start_remote_prog $new_host $CHECK_USER "echo" "\"hello $new_host\"" prg_exit_state $result 0 "" 1 0 ]
      }
   }

   if { $prg_exit_state != 0 } {
      puts $CHECK_OUTPUT "rlogin to host $new_host doesn't work correctly"
      return -1
   }
   if { [ string first "hello $new_host" $result ] < 0 } {
      puts $CHECK_OUTPUT "$result"
      puts $CHECK_OUTPUT "echo \"hello $new_host\" doesn't work"
      return -1
   }

   set arch [resolve_arch $new_host]
   lappend config(hostlist) $new_host

   
   set expect_bin [ start_remote_prog $new_host $CHECK_USER "which" "expect" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      set expect_bin "" 
   } 
   set vim_bin [ start_remote_prog $new_host $CHECK_USER "which" "vim" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      set vim_bin  "" 
   }
   set tar_bin [ start_remote_prog $new_host $CHECK_USER "which" "tar" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      set tar_bin "" 
   }
   set gzip_bin [ start_remote_prog $new_host $CHECK_USER "which" "gzip" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      set gzip_bin "" 
   }
   set ssh_bin [ start_remote_prog $new_host $CHECK_USER "which" "ssh" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      set ssh_bin "" 
   }

   set config($new_host,expect)        [string trim $expect_bin]
   set config($new_host,vim)           [string trim $vim_bin]
   set config($new_host,tar)           [string trim $tar_bin]
   set config($new_host,gzip)          [string trim $gzip_bin]
   set config($new_host,ssh)           [string trim $ssh_bin]
   set config($new_host,loadsensor)    ""
   set config($new_host,processors)    1
   set config($new_host,spooldir)      ""
   set config($new_host,arch)          $arch
   set config($new_host,compile)       0
   set config($new_host,compile_time)  0
   set config($new_host,response_time) [ expr ( [timestamp] - $time ) ]
   set config($new_host,fr_locale)     ""
   set config($new_host,ja_locale)     ""
   set config($new_host,zh_locale)     ""


   if { $have_host == "" } {
      host_config_hostlist_edit_host config $new_host
   }
      
   return 0   
}


#****** config/host/host_config_hostlist_edit_host() *********************************
#  NAME
#     host_config_hostlist_edit_host() -- edit host in host configuration
#
#  SYNOPSIS
#     host_config_hostlist_edit_host { array_name { has_host "" } } 
#
#  FUNCTION
#     This procedure is used for host edition in host configuration
#
#  INPUTS
#     array_name      - ts_host_config
#     { has_host "" } - if not "": just edit this host
#
#  SEE ALSO
#     check/setup_host_config()
#     check/verify_host_config()
#*******************************************************************************
proc host_config_hostlist_edit_host { array_name { has_host "" } } {
   global CHECK_OUTPUT
   global CHECK_USER 
   global ts_config ts_host_config

   upvar $array_name config

   set goto 0

   if { $has_host != "" } {
      set goto $has_host
   } 

   while { 1 } {

      clear_screen
      puts $CHECK_OUTPUT "\nEdit host in global host configuration"
      puts $CHECK_OUTPUT "======================================"

   
      host_config_hostlist_show_hosts config

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter hostname/number or return to exit: "
      if { $goto == 0 } {
         set host [wait_for_enter 1]
         set goto $host
      } else {
         set host $goto
         puts $CHECK_OUTPUT $host
      }
 
      if { [ string length $host ] == 0 } {
         break
      }
     
      if { [string is integer $host] } {
         incr host -1
         set host [ lindex $config(hostlist) $host ]
      }

      if { [ lsearch $config(hostlist) $host ] < 0 } {
         puts $CHECK_OUTPUT "host \"$host\" not found in list"
         wait_for_enter
         set goto 0
         continue
      }
      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT "   host          : $host"
      puts $CHECK_OUTPUT "   arch          : $config($host,arch)"
      puts $CHECK_OUTPUT "   expect        : $config($host,expect)"
      puts $CHECK_OUTPUT "   vim           : $config($host,vim)"
      puts $CHECK_OUTPUT "   tar           : $config($host,tar)"
      puts $CHECK_OUTPUT "   gzip          : $config($host,gzip)"
      puts $CHECK_OUTPUT "   ssh           : $config($host,ssh)"
      puts $CHECK_OUTPUT "   loadsensor    : $config($host,loadsensor)"
      puts $CHECK_OUTPUT "   processors    : $config($host,processors)"
      puts $CHECK_OUTPUT "   spooldir      : $config($host,spooldir)"
      puts $CHECK_OUTPUT "   fr_locale     : $config($host,fr_locale)"
      puts $CHECK_OUTPUT "   ja_locale     : $config($host,ja_locale)"
      puts $CHECK_OUTPUT "   zh_locale     : $config($host,zh_locale)"

      if { $config($host,compile) == 0 } {
         puts $CHECK_OUTPUT "   compile       : not a compile host"
      } else {
         puts $CHECK_OUTPUT "   compile       : testsuite will use this host to compile \"$config($host,arch)\" binaries"
      }
      puts $CHECK_OUTPUT "   compile_time  : $config($host,compile_time)"
      puts $CHECK_OUTPUT "   response_time : $config($host,response_time)"


      puts $CHECK_OUTPUT ""
   
      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter category to edit or hit return to exit > "
      set input [ wait_for_enter 1]
      if { [ string length $input ] == 0 } {
         set goto 0
         continue
      }

 
      if { [ string compare $input "host"] == 0 } {
         puts $CHECK_OUTPUT "Setting \"$input\" is not allowed"
         wait_for_enter
         continue
      }

      if { [ info exists config($host,$input) ] != 1 } {
         puts $CHECK_OUTPUT "Not a valid category"
         wait_for_enter
         continue
      }

      if { [string compare $input "arch"] == 0 } {
         puts $CHECK_OUTPUT "Setting \"$input\" is not allowed"
         wait_for_enter
         continue
      }


      set isfile 0
      set isdir 0
      set extra 0
      switch -- $input {
         "expect" { set isfile 1 }
         "vim"    { set isfile 1 }
         "tar"    { set isfile 1 }
         "gzip"   { set isfile 1 }
         "ssh"    { set isfile 1 }
         "loadsensor" { set isfile 1 }
         "spooldir" { set isdir 1 }
         "compile" { set extra 1 }
         "compile_time"  { set extra 2 }
         "response_time" { set extra 3 }
      }      

      set do_simple_test 0
      if { [string first "locale" $input] >= 0 } {
         puts $CHECK_OUTPUT "INFO:"
         puts $CHECK_OUTPUT "Please enter an environment list to get localized output on that host!"
         puts $CHECK_OUTPUT ""
         puts $CHECK_OUTPUT "e.g.: LANG=fr_FR.ISO8859-1 LC_MESSAGES=fr"
         set do_simple_test 1
      }

      if { $extra == 0 } {
         puts -nonewline $CHECK_OUTPUT "\nPlease enter new $input value: "
         set value [ wait_for_enter 1 ]
      }

      if { $extra == 1 } { ;# compile option
         puts -nonewline $CHECK_OUTPUT "\nShould testsuite use this host for compilation (y/n) :"
         set value [ wait_for_enter 1 ]
         if { [ string compare "y" $value ] == 0 } {
            set value 1
         } else {
            set value 0
         }
      }

      if { $extra == 2 || $extra == 3 } {
         puts $CHECK_OUTPUT "Setting \"$input\" is not allowed"
         wait_for_enter
         continue
      }

      if { $isfile } {
         set result [ start_remote_prog $host $CHECK_USER "ls" "$value" prg_exit_state 12 0 "" 1 0 ]
         if { $prg_exit_state != 0 } {
            puts $CHECK_OUTPUT $result
            puts $CHECK_OUTPUT "file $value not found on host $host"
            wait_for_enter
            continue
         }
      }
      
      if { $isdir } {
         set result [ start_remote_prog $host $CHECK_USER "cd" "$value" prg_exit_state 12 0 "" 1 0 ]
         if { $prg_exit_state != 0 } {
            puts $CHECK_OUTPUT $result
            puts $CHECK_OUTPUT "can't cd to directory $value on host $host"
            wait_for_enter
            continue
         }
      }

      if { $do_simple_test == 1 } {
         set mem_it $ts_host_config($host,$input)
         set mem_l10n $ts_config(l10n_test_locale)
       
         set ts_config(l10n_test_locale) [string range $input 0 1]
         set ts_host_config($host,$input) $value

         set test_result [perform_simple_l10n_test ]

         set ts_host_config($host,$input) $mem_it
         set ts_config(l10n_test_locale) mem_l10n

         if { $test_result != 0 } {
            puts $CHECK_OUTPUT "l10n errors" 
            wait_for_enter
            continue
         }
         puts $CHECK_OUTPUT "you have to enable l10n in testsuite setup too!"
         wait_for_enter
      }
      set config($host,$input) $value
   }
   
   return 0   
}


#****** config/host/host_config_hostlist_delete_host() *******************************
#  NAME
#     host_config_hostlist_delete_host() -- delete host from host configuration
#
#  SYNOPSIS
#     host_config_hostlist_delete_host { array_name } 
#
#  FUNCTION
#     This procedure is called to delete a host from host configuration
#
#  INPUTS
#     array_name - ts_host_config
#
#  SEE ALSO
#     check/setup_host_config()
#     check/verify_host_config()
#*******************************************************************************
proc host_config_hostlist_delete_host { array_name } {
   global CHECK_OUTPUT
   upvar $array_name config
   global CHECK_USER

   while { 1 } {

      clear_screen
      puts $CHECK_OUTPUT "\nDelete host from global host configuration"
      puts $CHECK_OUTPUT "=========================================="

   
      host_config_hostlist_show_hosts config

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter hostname/number or return to exit: "
      set host [wait_for_enter 1]
 
      if { [ string length $host ] == 0 } {
         break
      }
     
      if { [string is integer $host] } {
         incr host -1
         set host [ lindex $config(hostlist) $host ]
      }

      if { [ lsearch $config(hostlist) $host ] < 0 } {
         puts $CHECK_OUTPUT "host \"$host\" not found in list"
         wait_for_enter
         continue
      }

      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT "   host          : $host"
      puts $CHECK_OUTPUT "   arch          : $config($host,arch)"
      puts $CHECK_OUTPUT "   expect        : $config($host,expect)"
      puts $CHECK_OUTPUT "   vim           : $config($host,vim)"
      puts $CHECK_OUTPUT "   tar           : $config($host,tar)"
      puts $CHECK_OUTPUT "   gzip          : $config($host,gzip)"
      puts $CHECK_OUTPUT "   ssh           : $config($host,ssh)"
      puts $CHECK_OUTPUT "   loadsensor    : $config($host,loadsensor)"
      puts $CHECK_OUTPUT "   processors    : $config($host,processors)"
      puts $CHECK_OUTPUT "   spooldir      : $config($host,spooldir)"
      puts $CHECK_OUTPUT "   fr_locale     : $config($host,fr_locale)"
      puts $CHECK_OUTPUT "   ja_locale     : $config($host,ja_locale)"
      puts $CHECK_OUTPUT "   zh_locale     : $config($host,zh_locale)"

      if { $config($host,compile) == 0 } {
         puts $CHECK_OUTPUT "   compile       : not a compile host"
      } else {
         puts $CHECK_OUTPUT "   compile       : testsuite will use this host to compile \"$config($host,arch)\" binaries"
      }
      puts $CHECK_OUTPUT "   compile_time  : $config($host,compile_time)"
      puts $CHECK_OUTPUT "   response_time : $config($host,response_time)"


      puts $CHECK_OUTPUT ""
   
      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Delete this host? (y/n): "
      set input [ wait_for_enter 1]
      if { [ string length $input ] == 0 } {
         continue
      }

 
      if { [ string compare $input "y"] == 0 } {
         set index [lsearch $config(hostlist) $host]
         set config(hostlist) [ lreplace $config(hostlist) $index $index ]
         unset config($host,arch)
         unset config($host,expect)
         unset config($host,vim)
         unset config($host,tar)
         unset config($host,gzip)
         unset config($host,ssh)
         unset config($host,loadsensor)
         unset config($host,processors)
         unset config($host,spooldir)
         unset config($host,fr_locale)
         unset config($host,ja_locale)
         unset config($host,zh_locale)
         unset config($host,compile)
         unset config($host,compile_time)
         unset config($host,response_time)
         continue
      }
   }
   return 0   
}



#****** config/host/verify_host_config() *********************************************
#  NAME
#     verify_host_config() -- verify testsuite host configuration setup
#
#  SYNOPSIS
#     verify_host_config { config_array only_check parameter_error_list 
#     { force 0 } } 
#
#  FUNCTION
#     This procedure will verify or enter host setup configuration
#
#  INPUTS
#     config_array         - array name with configuration (ts_host_config)
#     only_check           - if 1: don't ask user, just check
#     parameter_error_list - returned list with error information
#     { force 0 }          - force ask user 
#
#  RESULT
#     number of errors
#
#  SEE ALSO
#     check/verify_host_config()
#     check/verify_user_config()
#     check/verify_config()
#     
#*******************************************************************************
proc verify_host_config { config_array only_check parameter_error_list { force 0 }} {
   global CHECK_OUTPUT actual_ts_host_config_version be_quiet
   upvar $config_array config
   upvar $parameter_error_list error_list

   set errors 0
   set error_list ""

   if { [ info exists config(version) ] != 1 } {
      puts $CHECK_OUTPUT "Could not find version info in host configuration file"
      lappend error_list "no version info"
      incr errors 1
      return -1
   }

   if { $config(version) != $actual_ts_host_config_version } {
      puts $CHECK_OUTPUT "Host configuration file version \"$config(version)\" not supported."
      puts $CHECK_OUTPUT "Expected version is \"$actual_ts_host_config_version\""
      lappend error_list "unexpected version"
      incr errors 1
      return -1
   } else {
      debug_puts "Host Configuration Version: $config(version)"
   }

   set max_pos [get_configuration_element_count config]

   set uninitalized ""
   if { $be_quiet == 0 } { 
      puts $CHECK_OUTPUT ""
   }
   for { set param 1 } { $param <= $max_pos } { incr param 1 } {
      set par [ get_configuration_element_name_on_pos config $param ]
      if { $be_quiet == 0 } { 
         puts -nonewline $CHECK_OUTPUT "      $config($par,desc) ..."
         flush $CHECK_OUTPUT
      }
      if { $config($par) == "" || $force != 0 } {
         debug_puts "not initialized or forced!"
         lappend uninitalized $param
         if { $only_check != 0 } {
            lappend error_list ">$par< configuration not initalized"
            incr errors 1
         }
      } else {
         set procedure_name  $config($par,setup_func)
         set default_value   $config($par,default)
         set description     $config($par,desc)
         if { [string length $procedure_name] == 0 } {
             debug_puts "no procedure defined"
         } else {
            if { [info procs $procedure_name ] != $procedure_name } {
               puts $CHECK_OUTPUT "error\n"
               puts $CHECK_OUTPUT "-->WARNING: unkown procedure name: \"$procedure_name\" !!!"
               puts $CHECK_OUTPUT "   ======="
               lappend uninitalized $param

               if { $only_check == 0 } { 
                  wait_for_enter 
               }
            } else {
               # call procedure only_check == 1
               debug_puts "starting >$procedure_name< (verify mode) ..."
               set value [ $procedure_name 1 $par config ]
               if { $value == -1 } {
                  incr errors 1
                  lappend error_list $par 
                  lappend uninitalized $param

                  puts $CHECK_OUTPUT "error\n"
                  puts $CHECK_OUTPUT "-->WARNING: verify error in procedure \"$procedure_name\" !!!"
                  puts $CHECK_OUTPUT "   ======="

               } 
            }
         }
      }
      if { $be_quiet == 0 } { 
         puts $CHECK_OUTPUT "\r      $config($par,desc) ... ok"   
      }
   }
   if { [set count [llength $uninitalized]] != 0 && $only_check == 0 } {
      puts $CHECK_OUTPUT "$count parameters are not initialized!"
      puts $CHECK_OUTPUT "Entering setup procedures ..."
      
      foreach pos $uninitalized {
         set p_name [get_configuration_element_name_on_pos config $pos]
         set procedure_name  $config($p_name,setup_func)
         set default_value   $config($p_name,default)
         set description     $config($p_name,desc)
       
         puts $CHECK_OUTPUT "----------------------------------------------------------"
         puts $CHECK_OUTPUT $description
         puts $CHECK_OUTPUT "----------------------------------------------------------"
         debug_puts "Starting configuration procedure for parameter \"$p_name\" ($config($p_name,pos)) ..."
         set use_default 0
         if { [string length $procedure_name] == 0 } {
            puts $CHECK_OUTPUT "no procedure defined"
            set use_default 1
         } else {
            if { [info procs $procedure_name ] != $procedure_name } {
               puts $CHECK_OUTPUT ""
               puts $CHECK_OUTPUT "-->WARNING: unkown procedure name: \"$procedure_name\" !!!"
               puts $CHECK_OUTPUT "   ======="
               if { $only_check == 0 } { wait_for_enter }
               set use_default 1
            }
         } 

         if { $use_default != 0 } {
            # check again if we have value ( force flag) 
            if { $config($p_name) == "" } {
               # we have no setup procedure
               if { $default_value != "" } {
                  puts $CHECK_OUTPUT "using default value: \"$default_value\"" 
                  set config($p_name) $default_value 
               } else {
                  puts $CHECK_OUTPUT "No setup procedure and no default value found!!!"
                  if { $only_check == 0 } {
                     puts -nonewline $CHECK_OUTPUT "Please enter value for parameter \"$p_name\": "
                     set value [wait_for_enter 1]
                     puts $CHECK_OUTPUT "using value: \"$value\"" 
                     set config($p_name) $value
                  } 
               }
            }
         } else {
            # call setup procedure ...
            debug_puts "starting >$procedure_name< (setup mode) ..."
            set value [ $procedure_name 0 $p_name config ]
            if { $value != -1 } {
               puts $CHECK_OUTPUT "using value: \"$value\"" 
               set config($p_name) $value
            }
         }
         if { $config($p_name) == "" } {
            puts $CHECK_OUTPUT "" 
            puts $CHECK_OUTPUT "-->WARNING: no value for \"$p_name\" !!!"
            puts $CHECK_OUTPUT "   ======="
            incr errors 1
            lappend error_list $p_name
         }
      } 
   }
   return $errors
}

#****** config/host/setup_host_config() **********************************************
#  NAME
#     setup_host_config() -- testsuite host configuration initalization
#
#  SYNOPSIS
#     setup_host_config { file { force 0 } } 
#
#  FUNCTION
#     This procedure will initalize the testsuite host configuration
#
#  INPUTS
#     file        - host configuration file
#     { force 0 } - if 1: edit configuration setup
#
#  SEE ALSO
#     check/setup_user_config()
#*******************************************************************************
proc setup_host_config { file { force 0 }} {
   global CHECK_OUTPUT
   global ts_host_config actual_ts_host_config_version

   if { [read_array_from_file $file "testsuite host configuration" ts_host_config ] == 0 } {
      if { $ts_host_config(version) != $actual_ts_host_config_version } {
         puts $CHECK_OUTPUT "unkown host configuration file version: $ts_host_config(version)"
         while { [update_ts_host_config_version $file] != 0 } {
            wait_for_enter
         }
      }
      # got config
      if { [verify_host_config ts_host_config 1 err_list $force ] != 0 } {
         # configuration problems
         foreach elem $err_list {
            puts $CHECK_OUTPUT "$elem"
         } 
         puts $CHECK_OUTPUT "Press enter to edit host setup configurations"
         set answer [wait_for_enter 1]

         set not_ok 1
         while { $not_ok } {
            if { [verify_host_config ts_host_config 0 err_list $force ] != 0 } {
               set not_ok 1
               foreach elem $err_list {
                  puts $CHECK_OUTPUT "error in: $elem"
               } 
               puts $CHECK_OUTPUT "try again? (y/n)"
               set answer [wait_for_enter 1]
               if { $answer == "n" } {
                  puts $CHECK_OUTPUT "Do you want to save your changes? (y/n)"
                  set answer [wait_for_enter 1]
                  if { $answer == "y" } {
                     if { [ save_host_configuration $file] != 0} {
                        puts $CHECK_OUTPUT "Could not save host configuration"
                        wait_for_enter
                     }
                  }
                  return
               } else {
                  continue
               }
            } else {
              set not_ok 0
            }
         }
         if { [ save_host_configuration $file] != 0} {
            puts $CHECK_OUTPUT "Could not save host configuration"
            wait_for_enter
            return
         }
      }
      if { $force == 1 } {
         if { [ save_host_configuration $file] != 0} {
            puts $CHECK_OUTPUT "Could not save host configuration"
            wait_for_enter
         }
      }
      return
   } else {
      puts $CHECK_OUTPUT "could not open host config file \"$file\""
      puts $CHECK_OUTPUT "press return to create new host configuration file"
      wait_for_enter 1
      if { [ save_host_configuration $file] != 0} {
         exit -1
      }
      setup_host_config $file
   }
}

