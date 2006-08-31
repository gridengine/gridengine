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
set    actual_ts_host_config_version "1.8"

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
   set ts_host_config($parameter,pos)        4

   set parameter "NFS-ROOT2NOBODY"
   set ts_host_config($parameter)            ""
   set ts_host_config($parameter,desc)       "NFS shared directory with root to nobody mapping"
   set ts_host_config($parameter,default)    ""
   set ts_host_config($parameter,setup_func) "host_config_$parameter"
   set ts_host_config($parameter,onchange)   "install"
   set ts_host_config($parameter,pos)        2

   set parameter "NFS-ROOT2ROOT"
   set ts_host_config($parameter)            ""
   set ts_host_config($parameter,desc)       "NFS shared directory with root read/write rights"
   set ts_host_config($parameter,default)    ""
   set ts_host_config($parameter,setup_func) "host_config_$parameter"
   set ts_host_config($parameter,onchange)   "install"
   set ts_host_config($parameter,pos)        3


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
          puts $CHECK_OUTPUT "(4)  try nslookup scan"
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



#****** config_host/host_config_NFS-ROOT2NOBODY() ******************************
#  NAME
#     host_config_NFS-ROOT2NOBODY() -- nfs spooling dir setup
#
#  SYNOPSIS
#     host_config_NFS-ROOT2NOBODY { only_check name config_array } 
#
#  FUNCTION
#     NFS directory which is mounted with root to user nobody mapping setup
#     - called from verify_host_config()
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
proc host_config_NFS-ROOT2NOBODY { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global fast_setup

   upvar $config_array config

   set actual_value  $config($name)
   set default_value $config($name,default)
   set description   $config($name,desc)
   set value $actual_value

   if { $actual_value == "" } {
      set value $default_value
   }

   if { $only_check == 0 } {
       puts $CHECK_OUTPUT "" 
       puts $CHECK_OUTPUT "Please specify a NFS shared directory where the root"
       puts $CHECK_OUTPUT "user is mapped to user nobody or press >RETURN< to"
       puts $CHECK_OUTPUT "use the default value."
       puts $CHECK_OUTPUT "(default: $value)"
       puts -nonewline $CHECK_OUTPUT "> "
       set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set value $input 
      } else {
         puts $CHECK_OUTPUT "using default value"
      }
   } 

   if {!$fast_setup} {
      if { ![file isdirectory $value] } {
         puts $CHECK_OUTPUT " Directory \"$value\" not found"
         return -1
      }
   }

   return $value
}


#****** config_host/host_config_NFS-ROOT2ROOT() ********************************
#  NAME
#     host_config_NFS-ROOT2ROOT() -- nfs spooling dir setup
#
#  SYNOPSIS
#     host_config_NFS-ROOT2ROOT { only_check name config_array } 
#
#  FUNCTION
#     NFS directory which is mounted with root to user root mapping setup 
#     - called from verify_host_config()
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
proc host_config_NFS-ROOT2ROOT { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global fast_setup

   upvar $config_array config

   set actual_value  $config($name)
   set default_value $config($name,default)
   set description   $config($name,desc)
   set value $actual_value

   if { $actual_value == "" } {
      set value $default_value
   }

   if { $only_check == 0 } {
       puts $CHECK_OUTPUT "" 
       puts $CHECK_OUTPUT "Please specify a NFS shared directory where the root"
       puts $CHECK_OUTPUT "user is NOT mapped to user nobody and has r/w access"
       puts $CHECK_OUTPUT "or press >RETURN< to use the default value."
       puts $CHECK_OUTPUT "(default: $value)"
       puts -nonewline $CHECK_OUTPUT "> "
       set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set value $input 
      } else {
         puts $CHECK_OUTPUT "using default value"
      }
   } 

   if {!$fast_setup} {
      if { ![file isdirectory $value] } {
         puts $CHECK_OUTPUT " Directory \"$value\" not found"
         return -1
      }
   }

   return $value
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
proc host_config_hostlist_show_hosts {array_name} {
   global ts_config CHECK_OUTPUT

   upvar $array_name config

   set hostlist [lsort -dictionary $config(hostlist)]

   puts $CHECK_OUTPUT "\nHost list:\n"
   if {[llength $hostlist] == 0 } {
      puts $CHECK_OUTPUT "no hosts defined"
   }

   set max_length 0
   foreach host $hostlist {
      if { [string length $host] > $max_length } {
         set max_length [string length $host]
      }
   }  


   set index 0
   foreach host $hostlist {
      incr index 1 

      set space ""
      for { set i 0 } { $i < [ expr ( $max_length - [ string length $host ]  ) ] } { incr i 1 } {  
          append space " "
      }
      set c_comp 0
      set j_comp 0
      set all_comp {}
      if {[host_conf_is_compile_host $host config]} {
         set c_comp 1
         lappend all_comp "c"
      }
      if {[host_conf_is_java_compile_host $host config]} {
         set j_comp 1
         lappend all_comp "java"
      }

      if {$c_comp || $j_comp} {
         set comp_host "(compile host: $all_comp)"
      } else {
         set comp_host "                      "
      }

      set conf_arch [host_conf_get_arch $host config]

      if { $index <= 9 } {
         puts $CHECK_OUTPUT "    $index) $host $space ($conf_arch) $comp_host"
      } else {
         puts $CHECK_OUTPUT "   $index) $host $space ($conf_arch) $comp_host"
      }
   }

   return $hostlist
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
   global ts_config CHECK_OUTPUT
   upvar $array_name config
   upvar $save_array back

   if {[info exists back]} {
      unset back
   }

   puts $CHECK_OUTPUT "\nCompile architecture list:\n"
   if {[llength $config(hostlist)] == 0} {
      puts $CHECK_OUTPUT "no hosts defined"
   }

   set index 0
  
   set max_arch_length 0
   foreach host $config(hostlist) {
      if {[host_conf_is_compile_host $host config]} {
         set arch [host_conf_get_arch $host config]
         if {$arch != "unsupported"} {
            lappend arch_list $arch
            set host_list($arch) $host
            set arch_length [string length $arch]
            if {$max_arch_length < $arch_length} {
               set max_arch_length $arch_length
            }
         }
      }
   }

   set arch_list [lsort $arch_list]
   foreach arch $arch_list {
      set host $host_list($arch)
      if {[host_conf_is_compile_host $host config]} {
         incr index 1 
         set back(count) $index
         set back($index,arch) [host_conf_get_arch $host config]
         set back($index,host) $host 

         set arch_length [string length $arch]
         if { $index <= 9 } {
            puts $CHECK_OUTPUT "    $index) $arch [get_spaces [expr ( $max_arch_length - $arch_length ) ]] ($host)"
         } else {
            puts $CHECK_OUTPUT "   $index) $arch [get_spaces [expr ( $max_arch_length - $arch_length ) ]] ($host)" 
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
   global ts_config CHECK_OUTPUT
   global CHECK_USER

   upvar $array_name config
  
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
      puts $CHECK_OUTPUT "host \"$new_host\" is already in list"
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

   set java_bin [ start_remote_prog $new_host $CHECK_USER "which" "java" prg_exit_state 12 0 "" 1 0 ]
   if { $prg_exit_state != 0 } {
      set java_bin "" 
   }

   set myenv(EN_QUIET) "1"
   set java15_bin [ start_remote_prog $new_host $CHECK_USER "/bin/csh" "-c \"source /vol2/resources/en_jdk15 ; which java\"" prg_exit_state 12 0 myenv 1 0 ]

   if { $prg_exit_state != 0 } {
      set java15_bin "" 
   }

   set config($new_host,expect)        [string trim $expect_bin]
   set config($new_host,vim)           [string trim $vim_bin]
   set config($new_host,tar)           [string trim $tar_bin]
   set config($new_host,gzip)          [string trim $gzip_bin]
   set config($new_host,ssh)           [string trim $ssh_bin]
   set config($new_host,java)          [string trim $java_bin]
   set config($new_host,java15)        [string trim $java15_bin]
   set config($new_host,loadsensor)    ""
   set config($new_host,processors)    1
   set config($new_host,spooldir)      ""
   set config($new_host,arch,53)       "unsupported"
   set config($new_host,arch,60)       "unsupported"
   set config($new_host,arch,65)       "unsupported"
   set config($new_host,arch,$ts_config(gridengine_version))          $arch
   set config($new_host,compile,53)    0
   set config($new_host,compile,60)    0
   set config($new_host,compile,65)    0
   set config($new_host,java_compile,53)    0
   set config($new_host,java_compile,60)    0
   set config($new_host,java_compile,65)    0
   set config($new_host,compile_time)  0
   set config($new_host,response_time) [ expr ( [timestamp] - $time ) ]
   set config($new_host,fr_locale)     ""
   set config($new_host,ja_locale)     ""
   set config($new_host,zh_locale)     ""
   set config($new_host,zones)         ""

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
   global ts_config ts_host_config CHECK_OUTPUT
   global CHECK_USER 

   upvar $array_name config

   set goto 0

   if { $has_host != "" } {
      set goto $has_host
   } 

   while { 1 } {
      clear_screen
      puts $CHECK_OUTPUT "\nEdit host in global host configuration"
      puts $CHECK_OUTPUT "======================================"

      set hostlist [host_config_hostlist_show_hosts config]

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter hostname/number or return to exit: "
      if { $goto == 0 } {
         set host [wait_for_enter 1]
         set goto $host
      } else {
         set host $goto
         puts $CHECK_OUTPUT $host
      }
 
      if {[string length $host] == 0} {
         break
      }
     
      if {[string is integer $host]} {
         incr host -1
         set host [lindex $hostlist $host]
      }

      if {[lsearch $hostlist $host] < 0} {
         puts $CHECK_OUTPUT "host \"$host\" not found in list"
         wait_for_enter
         set goto 0
         continue
      }

      set arch [host_conf_get_arch $host config]
      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT "   host          : $host"
      puts $CHECK_OUTPUT "   arch          : $arch"
      puts $CHECK_OUTPUT "   expect        : $config($host,expect)"
      puts $CHECK_OUTPUT "   vim           : $config($host,vim)"
      puts $CHECK_OUTPUT "   tar           : $config($host,tar)"
      puts $CHECK_OUTPUT "   gzip          : $config($host,gzip)"
      puts $CHECK_OUTPUT "   ssh           : $config($host,ssh)"
      puts $CHECK_OUTPUT "   java          : $config($host,java)"
      puts $CHECK_OUTPUT "   java15        : $config($host,java15)"
      puts $CHECK_OUTPUT "   loadsensor    : $config($host,loadsensor)"
      puts $CHECK_OUTPUT "   processors    : $config($host,processors)"
      puts $CHECK_OUTPUT "   spooldir      : $config($host,spooldir)"
      puts $CHECK_OUTPUT "   fr_locale     : $config($host,fr_locale)"
      puts $CHECK_OUTPUT "   ja_locale     : $config($host,ja_locale)"
      puts $CHECK_OUTPUT "   zh_locale     : $config($host,zh_locale)"
      puts $CHECK_OUTPUT "   zones         : $config($host,zones)"

      if {[host_conf_is_compile_host $host config]} {
         puts $CHECK_OUTPUT "   compile       : compile host for \"$arch\" binaries ($ts_config(gridengine_version))"
      } else {
         puts $CHECK_OUTPUT "   compile       : not a compile host"
      }
      if {[host_conf_is_java_compile_host $host config]} {
         puts $CHECK_OUTPUT "   java_compile  : compile host for java ($ts_config(gridengine_version))"
      } else {
         puts $CHECK_OUTPUT "   java_compile  : not a java compile host"
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

 
      set isfile 0
      set isdir 0
      set islocale 0
      set input_type "forbidden"
      switch -- $input {
         "expect" -
         "vim" -
         "tar" -
         "gzip" -
         "ssh" -
         "java" -
         "java15" -
         "loadsensor" { 
            set input_type "simple"
            set isfile 1
         }

         "spooldir" {
            set input_type "simple"
            set isdir 1 
         }

         "compile" -
         "java_compile" { 
            set input_type "compile"
         }

         "zones" { 
            set input_type "zones"
         }

         "fr_locale" -
         "ja_locale" -
         "zh_locale" { 
            set input_type "locale"
            set islocale 1
         }

         "processors" {
            set input_type "simple"
         }
   
         "arch" {
            set input_type "arch"
         }

         "host" -
         "compile_time" -
         "response_time" {
            puts $CHECK_OUTPUT "Setting \"$input\" is not allowed"
            wait_for_enter
            continue
         }
         default {
            puts $CHECK_OUTPUT "Not a valid category"
            wait_for_enter
            continue
         }
      }

      switch -exact $input_type {
         "simple" {
            puts -nonewline $CHECK_OUTPUT "\nPlease enter new $input value: "
            set value [wait_for_enter 1]
         }

         "arch" {
            set input "arch,$ts_config(gridengine_version)"
            puts $CHECK_OUTPUT "Please enter a valid architecture name"
            puts $CHECK_OUTPUT "or \"unsupported\", if the hosts architecture is not"
            puts $CHECK_OUTPUT "supported on Gridengine $ts_config(gridengine_version) systems"
            puts -nonewline $CHECK_OUTPUT "\nNew architecture: "
            set value [wait_for_enter 1]
         }

         "compile" {
            puts -nonewline $CHECK_OUTPUT "\nShould testsuite use this host for $input of $ts_config(gridengine_version) version (y/n) :"
            set input "$input,$ts_config(gridengine_version)"
            set value [wait_for_enter 1]
            if {[string compare "y" $value] == 0} {
               set value 1
            } else {
               set value 0
            }
         }
         "locale" {
            puts $CHECK_OUTPUT "INFO:"
            puts $CHECK_OUTPUT "Please enter an environment list to get localized output on that host!"
            puts $CHECK_OUTPUT ""
            puts $CHECK_OUTPUT "e.g.: LANG=fr_FR.ISO8859-1 LC_MESSAGES=fr"
            puts -nonewline $CHECK_OUTPUT "\nPlease enter new locale: "
            set value [wait_for_enter 1]
         }
         "zones" {
            puts $CHECK_OUTPUT "Please enter a space separated list of zones: "
            set value [wait_for_enter 1]

            if { [llength $value] != 0 } {
               set host_error 0
               foreach zone $value {
                  set result [ start_remote_prog $zone $CHECK_USER "id" "" prg_exit_state 12 0 "" 1 0 ]
                  if { $prg_exit_state != 0 } {
                     puts $CHECK_OUTPUT $result
                     puts $CHECK_OUTPUT "can't connect to zone $zone"
                     wait_for_enter
                     set host_error 1
                     break
                  }
               }
               if {$host_error} {
                  continue
               }
            }
         }
      }

      # check for valid file name
      if { $isfile } {
         set result [ start_remote_prog $host $CHECK_USER "ls" "$value" prg_exit_state 12 0 "" 1 0 ]
         if { $prg_exit_state != 0 } {
            puts $CHECK_OUTPUT $result
            puts $CHECK_OUTPUT "file $value not found on host $host"
            wait_for_enter
            continue
         }
      }
      
      # check for valid directory name
      if { $isdir } {
         set result [ start_remote_prog $host $CHECK_USER "cd" "$value" prg_exit_state 12 0 "" 1 0 ]
         if { $prg_exit_state != 0 } {
            puts $CHECK_OUTPUT $result
            puts $CHECK_OUTPUT "can't cd to directory $value on host $host"
            wait_for_enter
            continue
         }
      }

      # locale test
      if { $islocale == 1 } {
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
   global ts_config CHECK_OUTPUT
   global CHECK_USER

   upvar $array_name config

   while { 1 } {

      clear_screen
      puts $CHECK_OUTPUT "\nDelete host from global host configuration"
      puts $CHECK_OUTPUT "=========================================="

   
      set hostlist [host_config_hostlist_show_hosts config]

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter hostname/number or return to exit: "
      set host [wait_for_enter 1]
 
      if { [ string length $host ] == 0 } {
         break
      }
     
      if { [string is integer $host] } {
         incr host -1
         set host [lindex $hostlist $host]
      }

      if {[lsearch $hostlist $host] < 0} {
         puts $CHECK_OUTPUT "host \"$host\" not found in list"
         wait_for_enter
         continue
      }

      set arch [host_conf_get_arch $host config]
      
      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT "   host          : $host"
      puts $CHECK_OUTPUT "   arch          : $arch"
      puts $CHECK_OUTPUT "   expect        : $config($host,expect)"
      puts $CHECK_OUTPUT "   vim           : $config($host,vim)"
      puts $CHECK_OUTPUT "   tar           : $config($host,tar)"
      puts $CHECK_OUTPUT "   gzip          : $config($host,gzip)"
      puts $CHECK_OUTPUT "   ssh           : $config($host,ssh)"
      puts $CHECK_OUTPUT "   java          : $config($host,java)"
      puts $CHECK_OUTPUT "   loadsensor    : $config($host,loadsensor)"
      puts $CHECK_OUTPUT "   processors    : $config($host,processors)"
      puts $CHECK_OUTPUT "   spooldir      : $config($host,spooldir)"
      puts $CHECK_OUTPUT "   fr_locale     : $config($host,fr_locale)"
      puts $CHECK_OUTPUT "   ja_locale     : $config($host,ja_locale)"
      puts $CHECK_OUTPUT "   zh_locale     : $config($host,zh_locale)"

      if {[host_conf_is_compile_host $host config]} {
         puts $CHECK_OUTPUT "   compile       : compile host for \"$arch\" binaries ($ts_config(gridengine_version))"
      } else {
         puts $CHECK_OUTPUT "   compile       : not a compile host"
      }
      if {[host_conf_is_java_compile_host $host config]} {
         puts $CHECK_OUTPUT "   compile_java  : compile host for java ($ts_config(gridengine_version))"
      } else {
         puts $CHECK_OUTPUT "   compile_java  : not a java compile host"
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
         set config(hostlist) [lreplace $config(hostlist) $index $index]
         unset config($host,arch,53)
         unset config($host,arch,60)
         unset config($host,arch,65)
         unset config($host,expect)
         unset config($host,vim)
         unset config($host,tar)
         unset config($host,gzip)
         unset config($host,ssh)
         unset config($host,java)
         unset config($host,java15)
         unset config($host,loadsensor)
         unset config($host,processors)
         unset config($host,spooldir)
         unset config($host,fr_locale)
         unset config($host,ja_locale)
         unset config($host,zh_locale)
         unset config($host,compile,53)
         unset config($host,compile,60)
         unset config($host,compile,65)
         unset config($host,java_compile,53)
         unset config($host,java_compile,60)
         unset config($host,java_compile,65)
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

   if {[info exists config(version)] != 1} {
      puts $CHECK_OUTPUT "Could not find version info in host configuration file"
      lappend error_list "no version info"
      incr errors 1
      return -1
   }

   if {$config(version) != $actual_ts_host_config_version} {
      puts $CHECK_OUTPUT "Host configuration file version \"$config(version)\" not supported."
      puts $CHECK_OUTPUT "Expected version is \"$actual_ts_host_config_version\""
      lappend error_list "unexpected host config file version $config(version)"
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
proc setup_host_config {file {force 0}} {
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

#****** config/host/host_conf_get_nodes() **************************************
#  NAME
#     host_conf_get_nodes() -- return a list of exechosts and zones
#
#  SYNOPSIS
#     host_conf_get_nodes { host_list } 
#
#  FUNCTION
#     Iterates through host_list and builds a new node list, that contains
#     both hosts and zones.
#     If zones are available on a host, only the zones are contained in the new
#     node list,
#     if no zones are available on a host, the hostname will be contained in the
#     new node list.
#
#  INPUTS
#     host_list - list of physical hosts
#
#  RESULT
#     node list
#
#  SEE ALSO
#     config/host/host_conf_get_unique_nodes()
#*******************************************************************************
proc host_conf_get_nodes {host_list} {
   global ts_host_config

   set node_list {}

   foreach host $host_list {
      if {![info exists ts_host_config($host,zones)]} {
         add_proc_error "host_conf_get_nodes" -1 "host $host is not contained in testsuite host configuration!"
      } else {
         set zones $ts_host_config($host,zones)
         if {[llength $zones] == 0} {
            lappend node_list $host
         } else {
            set node_list [concat $node_list $zones]
         }
      }
   }

   return [lsort -dictionary $node_list]
}

#****** config/host/host_conf_get_unique_nodes() *******************************
#  NAME
#     host_conf_get_unique_nodes() -- return a unique list of exechosts and zones
#
#  SYNOPSIS
#     host_conf_get_unique_nodes { host_list } 
#
#  FUNCTION
#     Iterates through host_list and builds a new node list, that contains
#     both hosts and zones, but only one entry per physical host.
#     If zones are available on a host, only the first zone is contained in the new
#     node list,
#     if no zones are available on a host, the hostname will be contained in the
#     new node list.
#
#  INPUTS
#     host_list - list of physical hosts
#
#  RESULT
#     node list
#
#  SEE ALSO
#     config/host/host_conf_get_nodes()
#*******************************************************************************
proc host_conf_get_unique_nodes {host_list} {
   global ts_host_config

   set node_list {}

   foreach host $host_list {
      if {![info exists ts_host_config($host,zones)]} {
         add_proc_error "host_conf_get_unique_nodes" -1 "host $host is not contained in testsuite host configuration!"
      } else {
         set zones $ts_host_config($host,zones)
         if {[llength $zones] == 0} {
            lappend node_list $host
         } else {
            set node_list [concat $node_list [lindex $zones 0]]
         }
      }
   }

   return [lsort -dictionary $node_list]
}

proc host_conf_get_unique_arch_nodes {host_list} {
   global ts_host_config

   set node_list {}
   set covered_archs {}

   foreach node $host_list {
      set host [node_get_host $node]
      set arch [host_conf_get_arch $host]
      if {[lsearch -exact $covered_archs $arch] == -1} {
         lappend covered_archs $arch
         lappend node_list $node
      }
   }

   return $node_list
}

proc host_conf_get_all_nodes {host_list} {
   global ts_host_config

   set node_list {}

   foreach host $host_list {
      lappend node_list $host

      if {![info exists ts_host_config($host,zones)]} {
         add_proc_error "host_conf_get_all_nodes" -1 "host $host is not contained in testsuite host configuration!"
      } else {
         set zones $ts_host_config($host,zones)
         if {[llength $zones] > 0} {
            set node_list [concat $node_list $zones]
         }
      }
   }

   return [lsort -dictionary $node_list]
}

proc node_get_host {nodename} {
   global physical_host

   if {[info exists physical_host($nodename)]} {
      set ret $physical_host($nodename)
   } else {
      set ret $nodename
   }

   return $ret
}

proc node_set_host {nodename hostname} {
   global physical_host

   set physical_host($nodename) $hostname
}

proc node_get_processors {nodename} {
   global ts_host_config

   set host [node_get_host $nodename]

   return $ts_host_config($host,processors)
}

#****** config_host/host_conf_get_archs() **************************************
#  NAME
#     host_conf_get_archs() -- get all archs covered by a list of hosts
#
#  SYNOPSIS
#     host_conf_get_archs { nodelist } 
#
#  FUNCTION
#     Takes a list of hosts and returns a unique list of the architectures
#     of the hosts.
#
#  INPUTS
#     nodelist - list of nodes
#
#  RESULT
#     list of architectures
#*******************************************************************************
proc host_conf_get_archs {nodelist} {
   global ts_host_config

   set archs {}
   foreach node $nodelist {
      set host [node_get_host $node]
      lappend archs [host_conf_get_arch $host]
   }

   return [lsort -unique $archs]
}

#****** config_host/host_conf_get_arch_hosts() *********************************
#  NAME
#     host_conf_get_arch_hosts() -- find hosts of certain architectures
#
#  SYNOPSIS
#     host_conf_get_arch_hosts { archs } 
#
#  FUNCTION
#     Returns all hosts configured in the testuite host configuration,
#     that have one of the given architectures.
#
#  INPUTS
#     archs - list of architecture names
#
#  RESULT
#     list of hosts
#*******************************************************************************
proc host_conf_get_arch_hosts {archs} {
   global ts_host_config

   set hostlist {}

   foreach host $ts_host_config(hostlist) {
      if {[lsearch -exact $archs [host_conf_get_arch $host]] >= 0} {
         lappend hostlist $host
      }
   }

   return $hostlist
}

#****** config_host/host_conf_get_unused_host() ********************************
#  NAME
#     host_conf_get_unused_host() -- find a host not being referenced in our cluster
#
#  SYNOPSIS
#     host_conf_get_unused_host { {raise_error 1} } 
#
#  FUNCTION
#     Tries to find a host in the testsuite host configuration that
#     - is not referenced in the installed cluster (master/exec/submit/bdb_server)
#     - has an installed architecture
#
#  INPUTS
#     {raise_error 1} - raise an error (unsupported warning) if no such host is found
#
#  RESULT
#     The name of a host matching the above description.
#*******************************************************************************
proc host_conf_get_unused_host {{raise_error 1}} {
   global ts_config ts_host_config

   # return an empty string if we don't find a suited host
   set ret ""

   # get a list of all hosts referenced in the cluster
   set cluster_hosts [host_conf_get_cluster_hosts]

   # get a list of all configured hosts having one of the installed architectures
   set archs [host_conf_get_archs $cluster_hosts]
   set installed_hosts [host_conf_get_arch_hosts $archs]

   foreach host $installed_hosts {
      if {[lsearch -exact $cluster_hosts $host] == -1} {
         set ret $host
         break
      }
   }

   if {$ret == "" && $raise_error} {
      add_proc_error "host_conf_get_unused_host" -3 "cannot find an unused host having an installed architecture" 
   }

   return $ret
}

#****** config_host/get_java_home_for_host() **************************************************
#  NAME
#    get_java_home_for_host() -- Get the java home directory for a host
#
#  SYNOPSIS
#    get_java_home_for_host { host } 
#
#  FUNCTION
#     Reads the java home directory for a host from the host configuration
#
#  INPUTS
#    host -- name of the host
#
#  RESULT
#     
#     the java home directory of an empty string if the java is not set 
#     in the host configuration
#
#  EXAMPLE
#
#     set java_home [get_java_home_for_host $CHECK_HOST]
#
#     if { $java_home == "" } {
#         puts "java not configurated for host $CHECK_HOST"
#     }
#
#  NOTES
#     TODO: store JAVA_HOME in host config!
#
#  BUGS
#     Doesn't work for MAC OS X
#  SEE ALSO
#*******************************************************************************
proc get_java_home_for_host { host } {
   global ts_host_config CHECK_OUTPUT
   
    set input $ts_host_config($host,java)
    
    set input_len [ string length $input ]
    set java_len  [ string length "/bin/java" ]
    
    set last [ expr ( $input_len - $java_len -1 ) ]
    
    set res [ string range $input 0 $last]
    
    return $res
}

#****** config_host/get_java15_home_for_host() **************************************************
#  NAME
#    get_java15_home_for_host() -- Get the java15 home directory for a host
#
#  SYNOPSIS
#    get_java15_home_for_host { host } 
#
#  FUNCTION
#     Reads the java15 home directory for a host from the host configuration
#
#  INPUTS
#    host -- name of the host
#
#  RESULT
#     
#     the java15 home directory of an empty string if the java15 is not set 
#     in the host configuration
#
#  EXAMPLE
#
#     set java15_home [get_java15_home_for_host $CHECK_HOST]
#
#     if { $java15_home == "" } {
#         puts "java15 not configurated for host $CHECK_HOST"
#     }
#
#  NOTES
#     TODO: store JAVA_HOME in host config!
#
#  BUGS
#     Doesn't work for MAC OS X
#
#  SEE ALSO
#*******************************************************************************
proc get_java15_home_for_host { host } {
   global ts_host_config CHECK_OUTPUT
   
    set input $ts_host_config($host,java15)
    
    set input_len [ string length $input ]
    set java_len  [ string length "/bin/java" ]
    
    set last [ expr ( $input_len - $java_len -1 ) ]
    
    set res [ string range $input 0 $last]
    
    return $res
}


#****** config_host/host_conf_get_cluster_hosts() ******************************
#  NAME
#     host_conf_get_cluster_hosts() -- get a list of cluster hosts
#
#  SYNOPSIS
#     host_conf_get_cluster_hosts { } 
#
#  FUNCTION
#     Returns a list of all hosts that are part of the given cluster.
#     The list contains
#     - the master host
#     - the execd hosts
#     - the execd nodes (execd hosts with Solaris zones resolved)
#     - submit only hosts
#     - a berkeleydb RPC server host
#
#     The list is sorted by hostname, hostnames are unique.
#
#  RESULT
#     hostlist
#*******************************************************************************
proc host_conf_get_cluster_hosts {} {
   global ts_config CHECK_OUTPUT

   set hosts "$ts_config(master_host) $ts_config(execd_hosts) $ts_config(execd_nodes) $ts_config(submit_only_hosts) $ts_config(bdb_server)"
   set cluster_hosts [lsort -dictionary -unique $hosts]
   set none_elem [lsearch $cluster_hosts "none"]
   if {$none_elem >= 0} {
      set cluster_hosts [lreplace $cluster_hosts $none_elem $none_elem]
   }

   return $cluster_hosts
}

#****** config_host/host_conf_is_compile_host() ********************************
#  NAME
#     host_conf_is_compile_host() -- is a given host compile host?
#
#  SYNOPSIS
#     host_conf_is_compile_host { host {config_var ""} } 
#
#  FUNCTION
#     Returns if a given host is used as compile host.
#     The information is retrieved from the ts_host_config array,
#     unless another array is specified (e.g. during configuration).
#
#  INPUTS
#     host            - the host
#     {config_var ""} - configuration array, default ts_host_config
#
#  RESULT
#     0: is not compile host
#     1: is compile host
#*******************************************************************************
proc host_conf_is_compile_host {host {config_var ""}} {
   global ts_config ts_host_config CHECK_OUTPUT
   
   # we might work on a temporary config
   if {$config_var == ""} { 
      upvar 0 ts_host_config config
   } else {
      upvar 1 $config_var config
   }

   set ret 0

   if {[info exists config($host,compile,$ts_config(gridengine_version))]} {
      set ret $config($host,compile,$ts_config(gridengine_version))
   }

   return $ret
}

#****** config_host/host_conf_is_java_compile_host() ********************************
#  NAME
#     host_conf_is_java_compile_host() -- is a given host compile host for java?
#
#  SYNOPSIS
#     host_conf_is_java_compile_host { host {config_var ""} } 
#
#  FUNCTION
#     Returns if a given host is used as java compile host.
#     The information is retrieved from the ts_host_config array,
#     unless another array is specified (e.g. during configuration).
#
#  INPUTS
#     host            - the host
#     {config_var ""} - configuration array, default ts_host_config
#
#  RESULT
#     0: is not java compile host
#     1: is compile host
#*******************************************************************************
proc host_conf_is_java_compile_host {host {config_var ""}} {
   global ts_config ts_host_config CHECK_OUTPUT
   
   # we might work on a temporary config
   if {$config_var == ""} { 
      upvar 0 ts_host_config config
   } else {
      upvar 1 $config_var config
   }

   set ret 0

   if {[info exists config($host,java_compile,$ts_config(gridengine_version))]} {
      set ret $config($host,java_compile,$ts_config(gridengine_version))
   }

   return $ret
}

#****** config_host/host_conf_get_arch() ***************************************
#  NAME
#     host_conf_get_arch() -- return a host's architecture
#
#  SYNOPSIS
#     host_conf_get_arch { host {config_var ""} } 
#
#  FUNCTION
#     Returns the architecture that is configured in the testsuite
#     host configuration.
#     The architecture string may be Grid Engine version dependent, that
#     means, the function may return different architecture strings for
#     different Grid Engine version (53, 60, 65, ...).
#     If a host is not supported platform for a certain Grid Engine version,
#     "unsupported" is returned as archictecture name.
#
#  INPUTS
#     host            - the host
#     {config_var ""} - configuration array, default ts_host_config
#
#  RESULT
#     an architecture string, e.g. sol-sparc64, darwin, ...
#*******************************************************************************
proc host_conf_get_arch {host {config_var ""}} {
   global ts_config ts_host_config CHECK_OUTPUT
   
   # we might work on a temporary config
   if {$config_var == ""} { 
      upvar 0 ts_host_config config
   } else {
      upvar 1 $config_var config
   }

   set ret ""

   if {[info exists config($host,arch,$ts_config(gridengine_version))]} {
      set ret $config($host,arch,$ts_config(gridengine_version))
   }

   return $ret
}

#****** config_host/host_conf_is_known_host() **********************************
#  NAME
#     host_conf_is_known_host() -- is a host configured in testsuite host conf
#
#  SYNOPSIS
#     host_conf_is_known_host { host {config_var ""} } 
#
#  FUNCTION
#     Checks if a given host is configured in the testsuite host configuration.
#
#  INPUTS
#     host            - the host
#     {config_var ""} - configuration array, default ts_host_config
#
#  RESULT
#     0: host is not configured in testsuite host config
#     1: is a configured host
#
#  SEE ALSO
#     config_host/host_conf_is_supported_host()
#*******************************************************************************
proc host_conf_is_known_host {host {config_var ""}} {
   global ts_config ts_host_config CHECK_OUTPUT

   # we might work on a temporary config
   if {$config_var == ""} { 
      upvar 0 ts_host_config config
   } else {
      upvar 1 $config_var config
   }

   set ret 1

   if {[lsearch $config(hostlist) $host] < 0} {
      puts $CHECK_OUTPUT "Host \"$host\" is not in host configuration file"
      set ret 0
   }

   return $ret
}

#****** config_host/host_conf_is_supported_host() ******************************
#  NAME
#     host_conf_is_supported_host() -- is host supported for given GE version
#
#  SYNOPSIS
#     host_conf_is_supported_host { host {config_var ""} } 
#
#  FUNCTION
#     Checks if the given host is configured in the Grid Engine host
#     configuration and if it has an architecture, that is supported by the
#     given Grid Engine version.
#
#  INPUTS
#     host            - the host
#     {config_var ""} - configuration array, default ts_host_config
#
#  RESULT
#     0: host is not supported
#     1: is a supported host
#
#  SEE ALSO
#     config_host/host_conf_is_known_host()
#*******************************************************************************
proc host_conf_is_supported_host {host {config_var ""}} {
   global ts_config ts_host_config CHECK_OUTPUT

   # we might work on a temporary config
   if {$config_var == ""} { 
      upvar 0 ts_host_config config
   } else {
      upvar 1 $config_var config
   }

   set ret [host_conf_is_known_host $host config]

   if {$ret} {
      if {[host_conf_get_arch $host config] == "unsupported"} {
         puts $CHECK_OUTPUT "Host \"$host\" is not supported with Grid Engine $ts_config(gridengine_version)"
         set ret 0
      }
   }

   return $ret
}

#****** config_host/host_conf_53_arch() ****************************************
#  NAME
#     host_conf_53_arch() -- convert any arch string to 53 arch string
#
#  SYNOPSIS
#     host_conf_53_arch { arch } 
#
#  FUNCTION
#     Takes an architecture string and tries to convert it to a Grid Engine
#     5.3 architecture string.
#
#     If the given architecture string cannot be converted, "unknown" will
#     be returned.
#
#  INPUTS
#     arch - any arch string
#
#  RESULT
#     5.3 architecture string or "unknown"
#
#  SEE ALSO
#     config_host/host_conf_60_arch()
#     config_host/host_conf_65_arch()
#*******************************************************************************
proc host_conf_53_arch {arch} {
   switch -glob $arch {
      "sol-sparc" { return "solaris" }
      "sol-sparc64" { return "solaris64" }
      "sol-x86" { return "solaris86" }
      "lx??-x86" { return "glinux" }
      "lx??-alpha" { return "alinux" }
      "lx??-sparc" { return "slinux" }
      "lx??-ia64" { return "ia64linux" }
      "lx??-amd64" { return "lx24-amd64" }
      "irix65" { return "irix6" }

      "osf4" -
      "tru64" -
      "irix6" -
      "hp10" -
      "hp11" -
      "hp11-64" -
      "aix42" -
      "aix43" -
      "aix51" -
      "cray" -
      "crayts" -
      "craytsieee" -
      "necsx4" -
      "necsx5" -
      "sx" -
      "darwin" -
      "fbsd-*" -
      "nbsd-*" {
         return $arch
      }
   }

   return "unsupported"
}

#****** config_host/host_conf_60_arch() ****************************************
#  NAME
#     host_conf_60_arch() -- convert any arch string to 60 arch string
#
#  SYNOPSIS
#     host_conf_60_arch { arch } 
#
#  FUNCTION
#     Takes an architecture string and tries to convert it to a Grid Engine
#     6.0 architecture string.
#
#     If the given architecture string cannot be converted, "unknown" will
#     be returned.
#
#  INPUTS
#     arch - any arch string
#
#  RESULT
#     6.0 architecture string or "unknown"
#
#  SEE ALSO
#     config_host/host_conf_53_arch()
#     config_host/host_conf_65_arch()
#*******************************************************************************
proc host_conf_60_arch {arch} {
   # map old 5.3 names to 6.0
   # map 6.5 names to 6.0
   # allow all sol-, lx, fbsd, nbsd platforms for testsuite
   # allow selected architecture names
   # the rest will be unsupported
   switch -glob $arch {
      "solaris" { return "sol-sparc" }
      "solaris64" { return "sol-sparc64" }
      "solaris86" { return "sol-x86" }
      "glinux" { return "lx24-x86" }
      "alinux" { return "lx24-alpha" }
      "slinux" { return "lx24-sparc" }
      "ia64linux" { return "lx24-ia64" }
      "darwin-ppc" { return "darwin" }

      "sol-*" -
      "lx??-*" -
      "fbsd-*" -
      "nbsd-*" -

      "tru64" -
      "irix65" -
      "hp11" -
      "hp11-64" -
      "aix43" -
      "aix51" -
      "cray" -
      "crayts" -
      "craytsieee" -
      "craysmp" -
      "sx" -
      "darwin" -
      "win32-*" {
         return $arch
      }
   }

   return "unsupported"
}

#****** config_host/host_conf_65_arch() ****************************************
#  NAME
#     host_conf_65_arch() -- convert any arch string to 65 arch string
#
#  SYNOPSIS
#     host_conf_65_arch { arch } 
#
#  FUNCTION
#     Takes an architecture string and tries to convert it to a Grid Engine
#     6.5 architecture string.
#
#     If the given architecture string cannot be converted, "unknown" will
#     be returned.
#
#  INPUTS
#     arch - any arch string
#
#  RESULT
#     6.5 architecture string or "unknown"
#
#  SEE ALSO
#     config_host/host_conf_53_arch()
#     config_host/host_conf_60_arch()
#*******************************************************************************
proc host_conf_65_arch {arch} {
   # map old 5.3 names to 6.5
   # map 6.0 names to 6.5
   # allow all sol-, lx, fbsd, nbsd platforms for testsuite
   # allow selected architecture names
   # the rest will be unsupported
   switch -glob $arch {
      "solaris" { return "sol-sparc" }
      "solaris64" { return "sol-sparc64" }
      "solaris86" { return "sol-x86" }
      "glinux" { return "lx24-x86" }
      "alinux" { return "lx24-alpha" }
      "slinux" { return "lx24-sparc" }
      "ia64linux" { return "lx24-ia64" }

      "darwin" { return "darwin-ppc" }

      "sol-*" -
      "lx??-*" -
      "fbsd-*" -
      "nbsd-*" -

      "irix65" -
      "hp11" -
      "hp11-64" -
      "aix51" -
      "cray" -
      "crayts" -
      "craytsieee" -
      "craysmp" -
      "sx" -
      "darwin-ppc" -
      "darwin-x86" -
      "win32-*" {
         return $arch
      }
   }

   return "unsupported"
}

#****** config_host/host_conf_have_windows() ***********************************
#  NAME
#     host_conf_have_windows() -- do we have a windows host
#
#  SYNOPSIS
#     host_conf_have_windows { } 
#
#  FUNCTION
#     Returns whether we have a windows host in our testsuite cluster 
#     configuration.
#
#  RESULT
#     1 - if we have a windows host, else 0
#
#  SEE ALSO
#     config_host/host_conf_have_windows()
#     config_host/host_conf_get_cluster_hosts()
#     config_host/host_conf_get_arch()
#*******************************************************************************
proc host_conf_have_windows {} {
   set ret 0

   # get a list of all hosts referenced in the cluster
   set cluster_hosts [host_conf_get_cluster_hosts]

   # search for a windows host
   foreach host $cluster_hosts {
      if {[host_conf_get_arch $host] == "win32-x86"} {
         set ret 1
         break
      }
   }

   return $ret
}

#****** config_host/host_conf_get_windows_host() *******************************
#  NAME
#     host_conf_get_windows_host() -- get a windows host
#
#  SYNOPSIS
#     host_conf_get_windows_host { } 
#
#  FUNCTION
#     Returns the hostname of the first windows host in our testsuite cluster
#     configuration.
#
#  RESULT
#     A hostname of a windows host, or an empty string, if we don't have a 
#     windows host in our cluster.
#
#  SEE ALSO
#     config_host/host_conf_have_windows()
#     config_host/host_conf_get_cluster_hosts()
#     config_host/host_conf_get_arch()
#*******************************************************************************
proc host_conf_get_windows_host {} {
   set ret ""

   # get a list of all hosts referenced in the cluster
   set cluster_hosts [host_conf_get_cluster_hosts]
   
   # search and return the first windows host
   foreach host $cluster_hosts {
      if {[host_conf_get_arch $host] == "win32-x86"} {
         set ret $host
         break
      }
   }

   return $ret
}

#****** config_host/host_conf_get_windows_exec_host() **************************
#  NAME
#     host_conf_get_windows_exec_host() -- get a windows exec host
#
#  SYNOPSIS
#     host_conf_get_windows_exec_host { } 
#
#  FUNCTION
#     Returns the hostname of the first windows exec host in our testsuite
#     cluster configuration.
#
#  RESULT
#     A hostname of a windows exec host, or an empty string, if we don't have a 
#     windows exec host in our cluster.
#
#  SEE ALSO
#     config_host/host_conf_get_arch()
#*******************************************************************************
proc host_conf_get_windows_exec_host {} {
   global ts_config CHECK_OUTPUT
   set ret ""

   # get a list of all exec hosts referenced in the cluster
   set exec_hosts $ts_config(execd_nodes)

   # search for a windows host
   foreach host $exec_hosts {
      if {[host_conf_get_arch $host] == "win32-x86"} {
         set ret $host
         break
      }
   }

   return $ret
}

#****** config_host/host_conf_get_java_compile_host() **************************
#  NAME
#     host_conf_get_java_compile_host() -- get java compile host
#
#  SYNOPSIS
#     host_conf_get_java_compile_host { {raise_error 1} } 
#
#  FUNCTION
#     Returns the name of the java compile host configured in the host config.
#     If no compile host is found, an error is raised and an empty string
#     is returned.
#
#  INPUTS
#     {raise_error 1} - raise error condition or just output error message
#
#  RESULT
#     name of compile host or "", if no compile host was found
#
#  SEE ALSO
#     config_host/host_conf_is_java_compile_host()
#*******************************************************************************
proc host_conf_get_java_compile_host {{raise_error 1}} {
   global ts_config ts_host_config CHECK_OUTPUT

   set compile_host ""
   foreach host $ts_host_config(hostlist) {
      if {[host_conf_is_java_compile_host $host]} {
         set compile_host $host
         break
      }
   }

   if {$compile_host == ""} {
      add_proc_error "host_conf_get_java_compile_host" -1 "didn't find java compile host in host configuration" $raise_error
   }

   return $compile_host
}
