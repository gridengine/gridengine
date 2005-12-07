#!/vol2/TCL_TK/glinux/bin/expect --
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

proc set_options { opts } {
   global submit_options

   set submit_options $opts
   puts "OPTIONS OK"
}

proc set_script { opts } {
   global script

   set script $opts
   puts "SCRIPT OK"
}

proc submit_job { num_jobs add_opts } {
   global sge_root arch
   global submit_options script

   #puts "submitting job:"

   for {set i 0} {$i < $num_jobs} {incr i} {
      set start_clock [timestamp]
      set start_time [clock clicks -milliseconds]
      set args "exec $sge_root/bin/$arch/qsub $submit_options $add_opts $script"
      set result [catch $args output]
      set end_time [clock clicks -milliseconds]
      set end_clock [timestamp]
      if { $result == 0 } {
         set job_id [lindex $output 2]
         # array job?
         if {[string first "." $job_id] > 0} {
            set split_job_id [split $job_id "."]
            set job_id [lindex $split_job_id 0]
         }
         puts "SUBMIT OK $job_id [expr $end_time - $start_time] $start_clock $end_clock"
      } else {
         puts "SUBMIT FAILED $output"
      }
   }
}

proc get_arch {} {
   global sge_root

   if {[catch {exec "$sge_root/util/arch"} output] == 0} {
      #puts "architecture is $output"
      return $output
   } else {
      puts "ERROR: cannot evaluate architecture: $output"
      exit 1
   }
}

# MAIN
if { $argc != 2 } {
   puts "usage: $argv0 <SGE_ROOT> <HOST>"
   exit 1
}

set sge_root [lindex $argv 0]
set host     [lindex $argv 1]
set arch     [get_arch]
set submit_options ""
set script ""

# change dir to /tmp - and we have to submit with -cwd
# we had NFS problems, when 1500 jobs started and wanted to cd to /gridware/Testsuite
cd /tmp

puts "SUBMITTER $host"

set do_exit 0
log_user 0
while { !$do_exit } {
   expect_user {
      "QUIT\n" {
         set do_exit 1
         puts "QUIT OK $host"
      }
      "OPTIONS*\n" {
         set_options [lrange $expect_out(0,string) 1 end]
      }
      "SCRIPT*\n" {
         set_script [lrange $expect_out(0,string) 1 end]
      }
      "SUBMIT*\n" {
         submit_job [lindex $expect_out(0,string) 1] [lrange $expect_out(0,string) 2 end]
      }
      "*\n" {
         puts "ERROR: invalid input:  $expect_out(0,string)"
      }
   }
}
