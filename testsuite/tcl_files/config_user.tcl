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


global ts_user_config               ;# new testsuite user configuration array
global actual_ts_user_config_version      ;# actual user config version number
set    actual_ts_user_config_version "1.0"

if {![info exists ts_user_config]} {
   # ts_user_config defaults
   set parameter "version"
   set ts_user_config($parameter)            "$actual_ts_user_config_version"
   set ts_user_config($parameter,desc)       "Testuite user configuration setup"
   set ts_user_config($parameter,default)    "$actual_ts_user_config_version"
   set ts_user_config($parameter,setup_func) ""
   set ts_user_config($parameter,onchange)   "stop"
   set ts_user_config($parameter,pos)        1

   set parameter "userlist"
   set ts_user_config($parameter)            ""
   set ts_user_config($parameter,desc)       "Grid Engine cluster user list"
   set ts_user_config($parameter,default)    ""
   set ts_user_config($parameter,setup_func) "user_config_$parameter"
   set ts_user_config($parameter,onchange)   ""
   set ts_user_config($parameter,pos)        2

   set parameter "first_foreign_user"
   set ts_user_config($parameter)            ""
   set ts_user_config($parameter,desc)       "First testsuite cluster user name"
   set ts_user_config($parameter,default)    ""
   set ts_user_config($parameter,setup_func) "user_config_$parameter"
   set ts_user_config($parameter,onchange)   ""
   set ts_user_config($parameter,pos)        3

   set parameter "second_foreign_user"
   set ts_user_config($parameter)            ""
   set ts_user_config($parameter,desc)       "Second testsuite cluster user name"
   set ts_user_config($parameter,default)    ""
   set ts_user_config($parameter,setup_func) "user_config_$parameter"
   set ts_user_config($parameter,onchange)   ""
   set ts_user_config($parameter,pos)        4
    
   set parameter "first_foreign_group"
   set ts_user_config($parameter)            ""
   set ts_user_config($parameter,desc)       "First testsuite cluster user's group name"
   set ts_user_config($parameter,default)    ""
   set ts_user_config($parameter,setup_func) "user_config_$parameter"
   set ts_user_config($parameter,onchange)   ""
   set ts_user_config($parameter,pos)        5

   set parameter "second_foreign_group"
   set ts_user_config($parameter)            ""
   set ts_user_config($parameter,desc)       "Second testsuite cluster user's group name"
   set ts_user_config($parameter,default)    ""
   set ts_user_config($parameter,setup_func) "user_config_$parameter"
   set ts_user_config($parameter,onchange)   ""
   set ts_user_config($parameter,pos)        6
}

#****** config/user/user_config_first_foreign_user() *********************************
#  NAME
#     user_config_first_foreign_user() -- edit first foreign user
#
#  SYNOPSIS
#     user_config_first_foreign_user { only_check name config_array } 
#
#  FUNCTION
#     Testsuite user configuration setup - called from verify_user_config()
#
#  INPUTS
#     only_check   - 0: expect user input
#                    1: just verify user input
#     name         - option name (in ts_user_config array)
#     config_array - config array name (ts_config)
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_first_foreign_user { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global CHECK_FIRST_FOREIGN_SYSTEM_USER
   global CHECK_SECOND_FOREIGN_SYSTEM_USER
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
      # do setup
      puts $CHECK_OUTPUT "" 
      puts $CHECK_OUTPUT "Please enter name of the first testsuite user. This user must have access to the"
      puts $CHECK_OUTPUT "testsuite directory and must exist on all cluster hosts."
      puts $CHECK_OUTPUT "Press >RETURN< to accept the default value."
      puts $CHECK_OUTPUT "(default: $value)"
      puts -nonewline $CHECK_OUTPUT "> "
      set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set value $input 
      } else {
         puts $CHECK_OUTPUT "using default value"
      }
   } 

   # now verify
   if { ! $fast_setup } {
      if { [string compare $CHECK_USER $value] == 0 } {
         puts $CHECK_OUTPUT "first testsuite user \"$value\" and actual check user are identical!"
         return -1
      }
      if { [ info exists CHECK_SECOND_FOREIGN_SYSTEM_USER ] } {
         if { $CHECK_SECOND_FOREIGN_SYSTEM_USER == $value } {
            puts $CHECK_OUTPUT "first testsuite user should not be identical with second testsuite user"
            return -1
         }
      }
      
      set result [start_remote_prog $CHECK_HOST $CHECK_USER "id" "$value" prg_exit_state 60 0 "" 1 0]
      if { $prg_exit_state != 0 } {
         puts $CHECK_OUTPUT "id $value returns error. User $value not existing?"
         return -1
      }
   }

   # set global variables to value
   set CHECK_FIRST_FOREIGN_SYSTEM_USER $value

   return $value
}

#****** config/user/user_config_second_foreign_user() ********************************
#  NAME
#     user_config_second_foreign_user() -- setup second foreign user
#
#  SYNOPSIS
#     user_config_second_foreign_user { only_check name config_array } 
#
#  FUNCTION
#     Testsuite user configuration setup - called from verify_user_config()
#
#  INPUTS
#     only_check   - 0: expect user input
#                    1: just verify user input
#     name         - option name (in ts_user_config array)
#     config_array - config array name (ts_config)
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_second_foreign_user { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global CHECK_SECOND_FOREIGN_SYSTEM_USER
   global CHECK_FIRST_FOREIGN_SYSTEM_USER
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
      # do setup
      puts $CHECK_OUTPUT "" 
      puts $CHECK_OUTPUT "Please enter name of the second testsuite user. This user must have access to the"
      puts $CHECK_OUTPUT "testsuite directory and must exist on all cluster hosts."
      puts $CHECK_OUTPUT "Press >RETURN< to accept the default value."
      puts $CHECK_OUTPUT "(default: $value)"
      puts -nonewline $CHECK_OUTPUT "> "
      set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set value $input 
      } else {
         puts $CHECK_OUTPUT "using default value"
      }
   }

   # now verify
   if { ! $fast_setup } {
      if { [string compare $CHECK_USER $value] == 0 } {
         puts $CHECK_OUTPUT "second testsuite user \"$value\" and actual check user are identical!"
         return -1
      }
      if { [ info exists CHECK_FIRST_FOREIGN_SYSTEM_USER ] } {
         if { $value == $CHECK_FIRST_FOREIGN_SYSTEM_USER } {
            puts $CHECK_OUTPUT "second testsuite user should not be identical with first testsuite user"
            return -1
         }
      }
      set result [start_remote_prog $CHECK_HOST $CHECK_USER "id" "$value" prg_exit_state 60 0 "" 1 0]
      if { $prg_exit_state != 0 } {
         puts $CHECK_OUTPUT "id $value returns error. User $value not existing?"
         return -1
      }
   }

   # set global variables to value
   set CHECK_SECOND_FOREIGN_SYSTEM_USER $value

   return $value
}
#****** config/user/user_config_first_foreign_group() ********************************
#  NAME
#     user_config_first_foreign_group() -- first foreign user configuration setup
#
#  SYNOPSIS
#     user_config_first_foreign_group { only_check name config_array } 
#
#  FUNCTION
#     Testsuite user configuration setup - called from verify_user_config()
#
#  INPUTS
#     only_check   - 0: expect user input
#                    1: just verify user input
#     name         - option name (in ts_user_config array)
#     config_array - config array name (ts_config)
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_first_foreign_group { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global CHECK_FIRST_FOREIGN_SYSTEM_GROUP CHECK_FIRST_FOREIGN_SYSTEM_USER
   global CHECK_SECOND_FOREIGN_SYSTEM_GROUP
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
      # do setup
      puts $CHECK_OUTPUT "" 
      puts $CHECK_OUTPUT "Please enter the name of main group where user $CHECK_FIRST_FOREIGN_SYSTEM_USER is member."
      puts $CHECK_OUTPUT "Press >RETURN< to accept the default value."
      puts $CHECK_OUTPUT "(default: [ lindex $value 0 ])"
      puts -nonewline $CHECK_OUTPUT "> "
      set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set group1 $input 
      } else {
         set group1 [ lindex $value 0 ]
         puts $CHECK_OUTPUT "using default value"
      }
 
      puts $CHECK_OUTPUT "" 
      puts $CHECK_OUTPUT "Please enter the name of additional group where user $CHECK_FIRST_FOREIGN_SYSTEM_USER is member."
      puts $CHECK_OUTPUT "Press >RETURN< to accept the default value."
      puts $CHECK_OUTPUT "(default: [ lindex $value 1 ])"
      puts -nonewline $CHECK_OUTPUT "> "
      set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set group2 $input 
      } else {
         set group2 [ lindex $value 1 ]
         puts $CHECK_OUTPUT "using default value"
      }

      set value "$group1 $group2" 
   }

   # now verify
   if { ! $fast_setup } {
      if { [llength $value] != 2 } {
           puts $CHECK_OUTPUT "first testsuite user should belong to 2 groups"
           return -1
      }

      if { [info exists CHECK_SECOND_FOREIGN_SYSTEM_GROUP ] } {
         foreach gname $value {
            if { [ string compare $gname $CHECK_SECOND_FOREIGN_SYSTEM_GROUP] == 0 } {
                puts $CHECK_OUTPUT "first testsuite user should not have the same group as second testsuite user"
                return -1
            } 
         }
      }

      set group1 [lindex $value 0]
      set group2 [lindex $value 1]
      
      set result [start_remote_prog $CHECK_HOST $CHECK_USER "id" "$CHECK_FIRST_FOREIGN_SYSTEM_USER" prg_exit_state 60 0 "" 1 0]
      debug_puts $result
      if { [string first $group1 $result ] < 0 } {
         puts $CHECK_OUTPUT "first testsuite user ($CHECK_FIRST_FOREIGN_SYSTEM_USER) has not \"$group1\" as main group"
         return -1
      }

      set result [start_remote_prog $CHECK_HOST $CHECK_USER "groups" "$CHECK_FIRST_FOREIGN_SYSTEM_USER" prg_exit_state 60 0 "" 1 0]
      debug_puts $result
      if { $prg_exit_state == 0 } {
         if { [string first $group2 $result] < 0 } { 
            puts $CHECK_OUTPUT "first testsuite user ($CHECK_FIRST_FOREIGN_SYSTEM_USER) has not \"$group2\" as secondary group"
            return -1
         }
      }

      if { [llength $value] != 2 } {
           puts $CHECK_OUTPUT "first foreign system group must have 2 group entries"
           return -1
      }
   }


   # set global variables to value
   set CHECK_FIRST_FOREIGN_SYSTEM_GROUP $value

   return $value

}


#****** config/user/user_config_second_foreign_group() *******************************
#  NAME
#     user_config_second_foreign_group() -- setup second foreign group
#
#  SYNOPSIS
#     user_config_second_foreign_group { only_check name config_array } 
#
#  FUNCTION
#     Testsuite user configuration setup - called from verify_user_config()
#
#  INPUTS
#     only_check   - 0: expect user input
#                    1: just verify user input
#     name         - option name (in ts_user_config array)
#     config_array - config array name (ts_config)
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_second_foreign_group { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global CHECK_FIRST_FOREIGN_SYSTEM_GROUP CHECK_SECOND_FOREIGN_SYSTEM_USER
   global CHECK_SECOND_FOREIGN_SYSTEM_GROUP do_nomain
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
      # do setup
      puts $CHECK_OUTPUT "" 
      puts $CHECK_OUTPUT "Please enter the name of main group where user $CHECK_SECOND_FOREIGN_SYSTEM_USER is member."
      puts $CHECK_OUTPUT "Press >RETURN< to accept the default value."
      puts $CHECK_OUTPUT "(default: $value)"
      puts -nonewline $CHECK_OUTPUT "> "
      set input [ wait_for_enter 1]
      if { [ string length $input] > 0 } {
         set value $input 
      } else {
         puts $CHECK_OUTPUT "using default value"
      }
   }

   # now verify
   if { ! $fast_setup } {
      if { [llength $value] != 1 } {
           puts $CHECK_OUTPUT "second testsuite user should belong only to one group"
           return -1
      }

      if { [info exists CHECK_FIRST_FOREIGN_SYSTEM_GROUP ] } {
         foreach gname $CHECK_FIRST_FOREIGN_SYSTEM_GROUP {
            if { [ string compare $gname $value] == 0 } {
                puts $CHECK_OUTPUT "first testsuite user should not have the same group as second testsuite user"
                return -1
            } 
         }
      }

      set result [start_remote_prog $CHECK_HOST $CHECK_USER "id" "$CHECK_SECOND_FOREIGN_SYSTEM_USER" prg_exit_state 60 0 "" 1 0]
      debug_puts $result
      if { [string first $value $result ] < 0 && $do_nomain == 0 } {
         puts $CHECK_OUTPUT "second testsuite user ($CHECK_SECOND_FOREIGN_SYSTEM_USER) has not \"$value\" as main group"
         return -1
      }

      if { [llength $value] != 1 } {
           puts $CHECK_OUTPUT "second foreign system group must have 1 group entries"
           return -1
      }
   }

   # set global variables to value
   set CHECK_SECOND_FOREIGN_SYSTEM_GROUP $value

   return $value


}


#****** config/user/user_config_userlist() *******************************************
#  NAME
#     user_config_userlist() -- user list setup
#
#  SYNOPSIS
#     user_config_userlist { only_check name config_array } 
#
#  FUNCTION
#     Testsuite user configuration setup - called from verify_user_config()
#
#  INPUTS
#     only_check   - 0: expect user input
#                    1: just verify user input
#     name         - option name (in ts_user_config array)
#     config_array - config array name (ts_config)
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_userlist { only_check name config_array } {
   global CHECK_OUTPUT CHECK_HOST CHECK_USER
   global CHECK_REMOTE_ENVIRONMENT
   
   upvar $config_array config

   set description   $config($name,desc)

   if { $only_check == 0 } {
       set not_ready 1
       while { $not_ready } {
          clear_screen
          puts $CHECK_OUTPUT "\nGlobal user configuration setup"
          puts $CHECK_OUTPUT "==============================="
          puts $CHECK_OUTPUT "\n\n    users configured: [llength $config(userlist)]"
          user_config_userlist_show_users config
          puts $CHECK_OUTPUT "\n\n(1)  add user"
          puts $CHECK_OUTPUT "(2)  edit user"
          puts $CHECK_OUTPUT "(3)  delete user"
          puts $CHECK_OUTPUT "(10) exit setup"
          puts -nonewline $CHECK_OUTPUT "> "
          set input [ wait_for_enter 1]
          switch -- $input {
             1 {
                set result [user_config_userlist_add_user config]
                if { $result != 0 } {
                   wait_for_enter
                }
             }
             2 {
                set result [user_config_userlist_edit_user config]
                if { $result != 0 } {
                   wait_for_enter
                }
             }
             3 {
               set result [user_config_userlist_delete_user config]
                if { $result != 0 } {
                   wait_for_enter
                }
             }
             10 {
                set not_ready 0
             }
          } 
       }
   } 

   # check user configuration
   debug_puts "user_config_userlist:"
   foreach user $config(userlist) {
      debug_puts "checking user \"$user\" ... "
   }

   if { [ info exists config($CHECK_USER,envlist) ] } {
      set CHECK_REMOTE_ENVIRONMENT $config($CHECK_USER,envlist)
   } else {
      set CHECK_REMOTE_ENVIRONMENT ""
   }

   return $config(userlist)
}


#****** config/user/user_config_userlist_show_users() ********************************
#  NAME
#     user_config_userlist_show_users() -- show testsuite user configuration
#
#  SYNOPSIS
#     user_config_userlist_show_users { array_name } 
#
#  FUNCTION
#     This procedure will show the current testsuite user configuration
#
#  INPUTS
#     array_name - ts_user_config
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#*******************************************************************************
proc user_config_userlist_show_users { array_name } {
   global CHECK_OUTPUT
   upvar $array_name config

   puts $CHECK_OUTPUT "\nUser list:\n"
   if { [llength $config(userlist)] == 0 } {
      puts $CHECK_OUTPUT "no users defined"
   }
   set index 0
   foreach user $config(userlist) {
      incr index 1 
      puts $CHECK_OUTPUT "($index) $user (ports: $config($user,portlist))"
   }
}


#****** config/user/user_config_userlist_add_user() **********************************
#  NAME
#     user_config_userlist_add_user() -- add user to user configuration
#
#  SYNOPSIS
#     user_config_userlist_add_user { array_name { have_user "" } } 
#
#  FUNCTION
#     Add user to testsuite user configuration
#
#  INPUTS
#     array_name       - ts_user_config
#     { have_user "" } - if not "": add this user
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#*******************************************************************************
proc user_config_userlist_add_user { array_name { have_user "" } } {
   global CHECK_OUTPUT
   upvar $array_name config
   global CHECK_USER
  
   if { $have_user == "" } {
      clear_screen
      puts $CHECK_OUTPUT "\nAdd user to global user configuration"
      puts $CHECK_OUTPUT "====================================="

      user_config_userlist_show_users config

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter new username: "
      set new_user [wait_for_enter 1]
   } else {
      set new_user $have_user
   }

   if { [ string length $new_user ] == 0 } {
      puts $CHECK_OUTPUT "no username entered"
      return -1
   }
     
   if { [ lsearch $config(userlist) $new_user ] >= 0 } {
      puts $CHECK_OUTPUT "user \"$new_user\" is already in list"
      return -1
   }

   lappend config(userlist) $new_user
   set config($new_user,portlist) ""
   set config($new_user,envlist)  ""
   if { $have_user == "" } {
      user_config_userlist_edit_user config $new_user
   }
   return 0   
}

#****** config/user/user_config_userlist_edit_user() *********************************
#  NAME
#     user_config_userlist_edit_user() -- edit user configuration
#
#  SYNOPSIS
#     user_config_userlist_edit_user { array_name { has_user "" } } 
#
#  FUNCTION
#     This procedure is used to edit the testsuite user configuration
#
#  INPUTS
#     array_name      - ts_user_config
#     { has_user "" } - if not "": edit this user
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#*******************************************************************************
proc user_config_userlist_edit_user { array_name { has_user "" } } {
   global CHECK_OUTPUT
   upvar $array_name config
   global CHECK_USER CHECK_HOST
   global CHECK_REMOTE_ENVIRONMENT

   set goto 0

   if { $has_user != "" } {
      set goto $has_user
   } 

   while { 1 } {

      clear_screen
      puts $CHECK_OUTPUT "\nEdit user in global user configuration"
      puts $CHECK_OUTPUT "======================================"

   
      user_config_userlist_show_users config

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter username/number or return to exit: "
      if { $goto == 0 } {
         set user [wait_for_enter 1]
         set goto $user
      } else {
         set user $goto
         puts $CHECK_OUTPUT $user
      }
 
      if { [ string length $user ] == 0 } {
         break
      }
     
      if { [string is integer $user] } {
         incr user -1
         set user [ lindex $config(userlist) $user ]
      }

      if { [ lsearch $config(userlist) $user ] < 0 } {
         puts $CHECK_OUTPUT "user \"$user\" not found in list"
         wait_for_enter
         set goto 0
         continue
      }
      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT "   user     : $user"
      puts $CHECK_OUTPUT "   portlist : $config($user,portlist)"
      puts $CHECK_OUTPUT "   envlist  : $config($user,envlist)"
   
      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter category to edit or hit return to exit > "
      set input [ wait_for_enter 1]
      if { [ string length $input ] == 0 } {
         set goto 0
         continue
      }

 
      if { [ string compare $input "user"] == 0 } {
         puts $CHECK_OUTPUT "Setting \"$input\" is not allowed"
         wait_for_enter
         continue
      }

      if { [ info exists config($user,$input) ] != 1 } {
         puts $CHECK_OUTPUT "Not a valid category"
         wait_for_enter
         continue
      }

      set extra 0
      switch -- $input {
         "portlist"  { set extra 1 }
         "envlist"   { set extra 2 }
      }      

      if { $extra == 0 } {
         puts -nonewline $CHECK_OUTPUT "\nPlease enter new $input value: "
         set value [ wait_for_enter 1 ]
      }
      
      if { $extra == 1 } {
         puts -nonewline $CHECK_OUTPUT "\nPlease enter new $input value: "
         set value [ wait_for_enter 1 ]
         set errors [user_config_userlist_set_portlist config $user $value]
         if { $errors != 0 } {
            wait_for_enter
         }
         continue
      }

      if { $extra == 2 } {
         puts $CHECK_OUTPUT "The envlist has following syntax:"
         puts $CHECK_OUTPUT "variable=value \[...\] or local environment name to export e.g. DISPLAY"
         puts -nonewline $CHECK_OUTPUT "\nPlease enter new $input value: "
         set value [ wait_for_enter 1 ]
         set CHECK_REMOTE_ENVIRONMENT $value
         set back [ set_users_environment $CHECK_HOST]
         if { $back == 0 } {
            set config($user,$input) $value
         }
         wait_for_enter
         continue
      }


      set config($user,$input) $value
   }
   return 0   
}

#****** config/user/user_config_userlist_set_portlist() ******************************
#  NAME
#     user_config_userlist_set_portlist() -- set protlist for testsuite user
#
#  SYNOPSIS
#     user_config_userlist_set_portlist { array_name user value } 
#
#  FUNCTION
#     This procedure will set the portlist in the user configuration
#
#  INPUTS
#     array_name - ts_user_config
#     user       - user
#     value      - new portlist
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_userlist_set_portlist { array_name user value } {
   global ts_user_config
   global CHECK_OUTPUT

   upvar $array_name config

   set had_error 0 

   set ok_value ""
   set value [ lsort $value ]
         
   foreach port $value { 
      set had_error 0 
      if { [string is integer $port ] != 1 } {
         puts $CHECK_OUTPUT "$port is not a valid port number"
         set had_error 1
      } 
      if { [info exists config($port)] } {
         if { [ string compare $config($port) $user ] != 0 } {
            puts $CHECK_OUTPUT "user \"$config($port)\" has already reserved port $port"
            set had_error 1
         }
      } 
      if { [ lsearch -exact $ok_value $port ] >= 0 } {
          puts $CHECK_OUTPUT "ignoring double entry of port $port"
          set had_error 1
      }

      if { $had_error == 0 } {
         lappend ok_value $port
      } 
   }
   foreach port $config($user,portlist) {
      if { [lsearch -exact $ok_value $port] < 0 } {
         puts $CHECK_OUTPUT "removing port $port"
         unset config($port)
         unset config($port,$user) 
      }
   }
   set tmp_port_list ""
   foreach port $ok_value {
      set config($port) $user
      set config($port,$user) [user_config_userlist_create_gid_port config $port $user]
      set tmp_port_list "$tmp_port_list $port"
      set config($user,portlist) $tmp_port_list
   }
   set config($user,portlist) $ok_value
   return $had_error
}

#****** config/user/user_config_userlist_create_gid_port() ***************************
#  NAME
#     user_config_userlist_create_gid_port() -- create gid-range for user/port
#
#  SYNOPSIS
#     user_config_userlist_create_gid_port { array_name port user } 
#
#  FUNCTION
#     Create new gid-range for user/port combination
#
#  INPUTS
#     array_name - ts_user_config
#     port       - user port
#     user       - user
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#*******************************************************************************
proc user_config_userlist_create_gid_port { array_name port user } {
   global CHECK_OUTPUT
   upvar $array_name config

   if { [ info exists config($port,$user) ] } {
      puts $CHECK_OUTPUT "user $user ($port) gid_range: $config($port,$user)"
      return $config($port,$user)
   }
   
   

   set highest_gid_start 12800
   if { [info exist config(userlist)] } {
      set userlist $config(userlist)
      foreach user_loop $userlist {
         set portlist $config($user_loop,portlist)
         foreach port_loop $portlist {
            set range $config($port_loop,$user_loop)
            set start_range [split $range "-"]
            set start_range [lindex $start_range 0]
            if { $start_range > $highest_gid_start } {
               set highest_gid_start $start_range
            }
         }
      }
   }

   set gid_start $highest_gid_start
   incr gid_start 200
   set gid_end $gid_start
   incr gid_end 199
   set gid_range "$gid_start-$gid_end"
   puts $CHECK_OUTPUT "user $user ($port) gid_range: $gid_range"
   return $gid_range
}

#****** config/user/user_config_userlist_delete_user() *******************************
#  NAME
#     user_config_userlist_delete_user() -- delete user from user configuration
#
#  SYNOPSIS
#     user_config_userlist_delete_user { array_name } 
#
#  FUNCTION
#     This procedure is called to select an user from the user configuration and
#     delete it.
#
#  INPUTS
#     array_name - ts_user_config
#
#  SEE ALSO
#     check/setup_user_config()
#     check/verify_user_config()
#
#*******************************************************************************
proc user_config_userlist_delete_user { array_name } {
   global CHECK_OUTPUT
   upvar $array_name config
   global CHECK_USER

   while { 1 } {

      clear_screen
      puts $CHECK_OUTPUT "\nDelete user from global user configuration"
      puts $CHECK_OUTPUT "=========================================="

   
      user_config_userlist_show_users config

      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Please enter username/number or return to exit: "
      set user [wait_for_enter 1]
 
      if { [ string length $user ] == 0 } {
         break
      }
     
      if { [string is integer $user] } {
         incr user -1
         set user [ lindex $config(userlist) $user ]
      }

      if { [ lsearch $config(userlist) $user ] < 0 } {
         puts $CHECK_OUTPUT "user \"$user\" not found in list"
         wait_for_enter
         continue
      }

      puts $CHECK_OUTPUT ""
      puts $CHECK_OUTPUT "   user          : $user"
      puts $CHECK_OUTPUT "   portlist     : $config($user,portlist)"

      puts $CHECK_OUTPUT ""


      puts $CHECK_OUTPUT ""
   
      puts $CHECK_OUTPUT "\n"
      puts -nonewline $CHECK_OUTPUT "Delete this user? (y/n): "
      set input [ wait_for_enter 1]
      if { [ string length $input ] == 0 } {
         continue
      }

 
      if { [ string compare $input "y"] == 0 } {
         set index [lsearch $config(userlist) $user]
         set config(userlist) [ lreplace $config(userlist) $index $index ]

         foreach port $config($user,portlist) {
            puts $CHECK_OUTPUT "removing port $port"
            unset config($port)
         }
         unset config($user,portlist)
         wait_for_enter
         continue
      }
   }
   return 0   
}






#****** config/user/verify_user_config() *********************************************
#  NAME
#     verify_user_config() -- verify testsuite user configuration setup
#
#  SYNOPSIS
#     verify_user_config { config_array only_check parameter_error_list 
#     { force 0 } } 
#
#  FUNCTION
#     This procedure will verify or enter user setup configuration
#
#  INPUTS
#     config_array         - array name with configuration (ts_user_config)
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
proc verify_user_config { config_array only_check parameter_error_list { force 0 }} {
   global CHECK_OUTPUT actual_ts_user_config_version be_quiet
   upvar $config_array config
   upvar $parameter_error_list error_list

   set errors 0
   set error_list ""

   if { [ info exists config(version) ] != 1 } {
      puts $CHECK_OUTPUT "Could not find version info in user configuration file"
      lappend error_list "no version info"
      incr errors 1
      return -1
   }

   if { $config(version) != $actual_ts_user_config_version } {
      puts $CHECK_OUTPUT "User configuration file version \"$config(version)\" not supported."
      puts $CHECK_OUTPUT "Expected version is \"$actual_ts_user_config_version\""
      lappend error_list "unexpected version"
      incr errors 1
      return -1
   } else {
      debug_puts "User Configuration Version: $config(version)"
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
                  puts $CHECK_OUTPUT "error\n"
                  puts $CHECK_OUTPUT "-->WARNING: verify error in procedure \"$procedure_name\" !!!"
                  puts $CHECK_OUTPUT "   ======="
                  lappend uninitalized $param

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


#****** config/user/setup_user_config() **********************************************
#  NAME
#     setup_user_config() -- testsuite user configuration initalization
#
#  SYNOPSIS
#     setup_user_config { file { force 0 } } 
#
#  FUNCTION
#     This procedure will initalize the testsuite user configuration
#
#  INPUTS
#     file        - user configuration file
#     { force 0 } - if 1: edit configuration setup
#
#  SEE ALSO
#     check/setup_host_config()
#*******************************************************************************
proc setup_user_config { file { force 0 }} {
   global CHECK_OUTPUT
   global ts_user_config actual_ts_user_config_version do_nomain
   global fast_setup

   if { [read_array_from_file $file "testsuite user configuration" ts_user_config ] == 0 } {
      if { $ts_user_config(version) != $actual_ts_user_config_version } {
         puts $CHECK_OUTPUT "unkown user configuration file version: $ts_user_config(version)"
         exit -1
      }

      # got config
      if { $do_nomain == 0 } {
         if { [verify_user_config ts_user_config 1 err_list $force ] != 0 } {
            # configuration problems
            foreach elem $err_list {
               puts $CHECK_OUTPUT "$elem"
            } 
            set not_ok 1
            while { $not_ok } {
               if { [verify_user_config ts_user_config 0 err_list $force ] != 0 } {
                  set not_ok 1
                  puts $CHECK_OUTPUT "User configuration error. Stop."
                  foreach elem $err_list {
                     puts $CHECK_OUTPUT "error in: $elem"
                  } 
                  puts $CHECK_OUTPUT "try again? (y/n)"
                  set answer [wait_for_enter 1]
                  if { $answer == "n" } {
                     puts $CHECK_OUTPUT "Do you want to save your changes? (y/n)"
                     set answer [wait_for_enter 1]
                     if { $answer == "y" } {
                        if { [ save_user_configuration $file] != 0} {
                           puts $CHECK_OUTPUT "Could not save user configuration"
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
            if { [ save_user_configuration $file] != 0} {
               puts $CHECK_OUTPUT "Could not save user configuration"
               wait_for_enter
               return
            }

         }
         if { $force == 1 } {
            if { [ save_user_configuration $file] != 0} {
               puts $CHECK_OUTPUT "Could not save user configuration"
               wait_for_enter
            }
         }
         return
      }
      return 
   } else {
      puts $CHECK_OUTPUT "could not open user config file \"$file\""
      puts $CHECK_OUTPUT "press return to create new user configuration file"
      wait_for_enter 1
      if { [ save_user_configuration $file] != 0} {
         exit -1
      }
      setup_user_config $file
   }
}

#****** config_user/user_conf_get_cluster_users() ******************************
#  NAME
#     user_conf_get_cluster_users() -- get a list of cluster users
#
#  SYNOPSIS
#     user_conf_get_cluster_users { } 
#
#  FUNCTION
#     Returns a list of all users that will be used in the given test cluster.
#     The lists consists of
#     - the CHECK_USER
#     - root
#     - first and second "foreign" user
#
#  RESULT
#     user list
#*******************************************************************************
proc user_conf_get_cluster_users {} {
   global ts_user_config CHECK_USER

   set user_list $CHECK_USER
   lappend user_list "root"
   lappend user_list $ts_user_config(first_foreign_user)
   lappend user_list $ts_user_config(second_foreign_user)

   return $user_list
}
