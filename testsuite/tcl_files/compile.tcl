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

#****** compile/compile_check_compile_hosts() **********************************
#  NAME
#     compile_check_compile_hosts() -- check for suited compile host
#
#  SYNOPSIS
#     compile_check_compile_hosts { host_list } 
#
#  FUNCTION
#     Goes through the given host list and for every host checks,
#     if a compile host for the architecture of the host is defined
#     in the testsuite host configuration.
#
#  INPUTS
#     host_list - list of hosts to check
#
#  RESULT
#     0 - OK, compile hosts for all given hosts exist
#    -1 - at least for one host, no compile host is configured
#*******************************************************************************
proc compile_check_compile_hosts {host_list} {
   global ts_config ts_host_config

   # remember already resolved compile archs
   set compile_archs {}

   # check each host in host_list
   foreach host $host_list {
      if {![host_conf_is_supported_host $host]} {
         add_proc_error "compile_check_compile_hosts" -1 "host $host is not contained in testsuite host configuration or not supported host!"
      } else {
         # host's architecture
         set arch [host_conf_get_arch $host]

         # do we already have a compile host for this arch?
         # if not, search it.
         if {[lsearch $compile_archs $arch] < 0} {
            if {[compile_search_compile_host $arch] != "none"} {
               lappend compile_archs $arch
            } else {
               return -1
            }
         }
      }
   }

   return 0
}

#****** compile/compile_host_list() ********************************************
#  NAME
#     compile_host_list() -- build compile host list
#
#  SYNOPSIS
#     compile_host_list { } 
#
#  FUNCTION
#     Builds a list of compile host for all the architectures that are 
#     required to install the configured test cluster.
#
#     Takes into account the
#     - master host
#     - execd hosts
#     - shadowd hosts
#     - submit only hosts
#     - berkeley db rpc server host
#
#  RESULT
#     list of compile hosts
#     in case of errors, an empty list is returned
#
#  SEE ALSO
#     compile/compile_search_compile_host()
#*******************************************************************************
proc compile_host_list {} {
   global ts_config ts_host_config
   global CHECK_OUTPUT
   
   set host_list [concat $ts_config(master_host) $ts_config(execd_hosts) \
                         $ts_config(shadowd_hosts) $ts_config(submit_only_hosts) \
                         $ts_config(bdb_server) \
                         [checktree_get_required_hosts]]
   

   set host_list [compile_unify_host_list $host_list]

   foreach host $host_list {
      set arch [host_conf_get_arch $host]
      if {$arch == ""} {
         add_proc_error "compile_host_list" -1 "Can't not determine the architecture of host $host"
         return ""
      }
      if { ! [info exists compile_host($arch)] } {
         set c_host [compile_search_compile_host $arch]
         if {$c_host == "none"} {
            return ""
         } else {
            set compile_host($arch) $c_host
            lappend compile_host(list) $c_host
         }
      }
   }


   return [lsort -dictionary $compile_host(list)]
}


#****** compile/get_compile_options_string() ***********************************
#  NAME
#     get_compile_options_string() -- return current compile option string
#
#  SYNOPSIS
#     get_compile_options_string { } 
#
#  FUNCTION
#     This function returns a string containing the current set aimk compile
#     options
#
#  RESULT
#     string containing compile options
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc get_compile_options_string { } {
   global ts_config CHECK_OUTPUT

   set options $ts_config(aimk_compile_options)

   if { $options == "none" } {
      set options ""
   }

   if { $options != "" } {
      puts $CHECK_OUTPUT "compile options are: \"$options\""
   }

   return $options
}

#****** compile/compile_unify_host_list() **************************************
#  NAME
#     compile_unify_host_list() -- remove duplicates and "none" from list
#
#  SYNOPSIS
#     compile_unify_host_list { host_list } 
#
#  FUNCTION
#     Takes a hostlist and removes all duplicate entries as well as 
#     "none" entries from it.
#     The resulting list is sorted.
#
#  INPUTS
#     host_list - list containing duplicates
#
#  RESULT
#     unified and sorted list
#*******************************************************************************
proc compile_unify_host_list {host_list} {
   set new_host_list {}

   # go over input host list
   foreach host $host_list {
      # filter out "none" entries (coming from empty lists)
      if {$host != "none"} {
         # if we don't have this host in output list, append it
         if {[lsearch $new_host_list $host] < 0} {
            lappend new_host_list $host
         }
      }
   }

   # return sorted list
   return [lsort -dictionary $new_host_list]
}

#****** compile/compile_search_compile_host() **********************************
#  NAME
#     compile_search_compile_host() -- search compile host by architecture
#
#  SYNOPSIS
#     compile_search_compile_host { arch } 
#
#  FUNCTION
#     Search the testsuite host configuration for a compile host for a 
#     certain architecture.
#
#  INPUTS
#     arch - required architecture
#
#  RESULT
#     name of the compile host
#     "none", if no compile host for the given architecture is defined
#*******************************************************************************
proc compile_search_compile_host {arch} {
   global ts_host_config
   global CHECK_OUTPUT

   foreach host $ts_host_config(hostlist) {
      if {[host_conf_get_arch $host] == $arch && \
          [host_conf_is_compile_host $host]} {
         return $host
      }
   }

   # no compile host found for this arch
   puts $CHECK_OUTPUT "no compile host found for architecture $arch"
   return "none"
}

