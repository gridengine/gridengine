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


#****** checktree_helper/exec_compile_hooks() **************************************************
#  NAME
#    exec_compile_hooks() -- execute a compile hooks
#
#  SYNOPSIS
#    exec_compile_hooks { compile_hosts report_nr } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#    compile_hosts --  list of all compile hosts
#    report_nr     --  id of the report
#
#  RESULT
#     0   --  all compile hooks are executed
#     > 0 --  number of failed compile hooks
#     -1  --  a compile has not been found
#
#  EXAMPLE
#
#  NOTES
#
#  BUGS
#
#  SEE ALSO
#
#*******************************************************************************
proc exec_compile_hooks { compile_hosts report_nr } {
   
   global ts_checktree CHECK_OUTPUT

   set error_count 0
   for {set i 0} { $i < $ts_checktree(act_nr)} {incr i 1 } {
      for {set ii 0} {[info exists ts_checktree($i,compile_hooks_${ii})]} {incr ii 1} {
         
         set compile_proc $ts_checktree($i,compile_hooks_${ii})
         
         if { [info procs $compile_proc ] != $compile_proc } {
            report_add_message $report_nr "Can not execute compile hook ${ii} of checktree $ts_checktree($i,dir_name), compile proc not found"
            return -1
         } else {
            set res [$compile_proc $compile_hosts $report_nr]
            if { $res != 0 } {
               report_add_message $report_nr "compile hook ${ii}  of checktree  $ts_checktree($i,dir_name) failed, $compile_proc returned $res\n"
               incr error_count
            }
         }
      }
   }
   return $error_count
}

#****** checktree_helper/exec_compile_clean_hooks() **************************************************
#  NAME
#    exec_compile_clean_hooks() -- execute a compile clean hook
#
#  SYNOPSIS
#    exec_compile_clean_hooks { compile_hosts report_nr } 
#
#  FUNCTION
#     This method executes all registered compile_clean hooks of the
#     checktree
#
#  INPUTS
#    compile_hosts -- list of compile hosts
#    report_nr     -- id if the report
#
#  RESULT
#     0   -- all compile_clean hooks has been executed
#    >0   -- number of failed compile_clean hooks
#    <0   -- configuration error
#
#  EXAMPLE
#
#  NOTES
#
#  BUGS
#
#  SEE ALSO
#*******************************************************************************
proc exec_compile_clean_hooks { compile_hosts report_nr } {
   
   global ts_checktree CHECK_OUTPUT

   set error_count 0
   for {set i 0} { $i < $ts_checktree(act_nr)} {incr i 1 } {
      for {set ii 0} {[info exists ts_checktree($i,compile_clean_hooks_${ii})]} {incr ii 1} {
         
         set compile_proc $ts_checktree($i,compile_clean_hooks_${ii})
         
         if { [info procs $compile_proc ] != $compile_proc } {
            report_add_message $report_nr "Can not execute compile_clean hook ${ii} of checktree $ts_checktree($i,dir_name), compile proc not found"
            return -1
         } else {
            set res [$compile_proc $compile_hosts $report_nr]
            if { $res != 0 } {
               report_add_message $report_nr "compile_clean hook ${ii}  of checktree  $ts_checktree($i,dir_name) failed, $compile_proc returned $res\n"
               incr error_count
            }
         }
      }
   }
   return $error_count
}




#****** checktree_helper/exec_install_binaries_hooks() **************************************************
#  NAME
#    exec_install_binaries_hooks() -- ???
#
#  SYNOPSIS
#    exec_install_binaries_hooks { } 
#
#  FUNCTION
#     Execute all registered install_binaries_hooks 
#
#  INPUTS
#    arch_list   -- list of architectures
#    report_nr   -- id of the report
#
#  RESULT
#     0  - on success
#     >1 - nuber of failed install_binaries_hooks
#     <0 - failure
#
#  EXAMPLE
#
#  NOTES
#
#  BUGS
#
#  SEE ALSO
#*******************************************************************************
proc exec_install_binaries_hooks { arch_list report_nr } {
   
   global ts_checktree CHECK_OUTPUT

   set error_count 0
   for {set i 0} { $i < $ts_checktree(act_nr)} {incr i 1 } {
      for {set ii 0} {[info exists ts_checktree($i,install_binary_hooks_${ii})]} {incr ii 1} {
         
         set prog $ts_checktree($i,install_binary_hooks_${ii})
         
         if { [info procs $prog ] != $prog } {
            add_proc_error "exec_install_binaries_hooks" -1 "Can not execute compile hook $ts_checktree($i,install_binary_hooks_${ii}), compile prog not found"
            return -1
         } else {
            set res [$prog $arch_list $report_nr]
            if { $res != 0 } {
               report_add_message $report_nr "install hook ${ii} of checktree  $ts_checktree($i,dir_name), $prog returned $res\n"
               incr error_count
            }
         }
      }
   }
   return $error_count
}


#****** checktree_helper/exec_shutdown_hooks() **************************************************
#  NAME
#    exec_shutdown_hooks() -- execute all shutdown hooks
#
#  SYNOPSIS
#    exec_shutdown_hooks { } 
#
#  FUNCTION
#     Executes all registered shutdown hooks
#
#  INPUTS
#
#  RESULT
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
proc exec_shutdown_hooks {} {
   
   global ts_checktree CHECK_OUTPUT

   set error_count 0
   for {set i 0} { $i < $ts_checktree(act_nr)} {incr i 1 } {
      for {set ii 0} {[info exists ts_checktree($i,shutdown_hooks_${ii})]} {incr ii 1} {
         
         set shutdown_hook $ts_checktree($i,shutdown_hooks_${ii})
         
         if { [info procs $shutdown_hook ] != $shutdown_hook } {
            puts $CHECK_OUTPUT "Can not execute shutdown hook ${ii} of checktree $ts_checktree($i,dir_name), shutdown proc not found"
            return -1
         } else {
            set res [$shutdown_hook]
            if { $res != 0 } {
               puts $CHECK_OUTPUT "shutdown hook ${ii}  of checktree  $ts_checktree($i,dir_name) failed, $shutdown_hook returned $res\n"
               incr error_count
            }
         }
      }
   }
   return $error_count
}


#****** checktree_helper/exec_startup_hooks() **************************************************
#  NAME
#    exec_startup_hooks() -- execute all startup hooks
#
#  SYNOPSIS
#    exec_startup_hooks { } 
#
#  FUNCTION
#     Executes all registered startup hooks
#     Additional checktree will be informed that the cluster starts up
#
#  INPUTS
#
#  RESULT
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
proc exec_startup_hooks {} {
   
   global ts_checktree CHECK_OUTPUT

   set error_count 0
   for {set i 0} { $i < $ts_checktree(act_nr)} {incr i 1 } {
      for {set ii 0} {[info exists ts_checktree($i,startup_hooks_${ii})]} {incr ii 1} {
         
         set startup_hook $ts_checktree($i,startup_hooks_${ii})
         
         if { [info procs $startup_hook ] != $startup_hook } {
            add_proc_error "exec_startup_hooks" -1 "Can not execute startup hook ${ii} of checktree $ts_checktree($i,dir_name), startup proc not found"
            return -1
         } else {
            set res [$startup_hook]
            if { $res != 0 } {
               add_proc_error "exec_startup_hooks" -1 "startup hook ${ii}  of checktree  $ts_checktree($i,dir_name) failed, $startup_hook returned $res\n"
               incr error_count
            }
         }
      }
   }
   return $error_count
}
