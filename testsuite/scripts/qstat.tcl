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

proc do_qstat {} {
   global active interval sge_root arch

   set start_time [clock clicks -milliseconds]
   if {$active} {
      set result [catch {exec "/bin/sh" "-c" "$sge_root/bin/$arch/qstat -f >/dev/null 2>&1"} output]
      # puts $output
      set end_time [clock clicks -milliseconds]
      set end_clock [clock seconds]
      set duration [expr double($end_time - $start_time) / 1000.0]
      if {$result == 0} {
         puts "QSTAT DATA $end_clock $duration"
      } else {
         puts "QSTAT ERROR $end_clock $duration"
      }
   }
   set end_time [clock clicks -milliseconds]

   # return the number of ms to next qstat
   set delay [expr $interval - ($end_time - $start_time)]
   if {$delay > 0} {
      set milli [expr $delay % 1000]
      set delay [expr $delay - $milli]

      # expect only knows timeouts in seconds.
      # so sleep for the milliseconds, expect will timeout after the seconds 
      # part
      after $milli
   } else {
      set delay 0
   }

   return $delay
}

# MAIN
if { $argc != 1 } {
   puts "usage: $argv0 <SGE_ROOT>"
   exit 1
}

set sge_root [lindex $argv 0]
set arch     [get_arch]

set active 0            ;# do measurements
set interval 1000       ;# default info interval: 1000 milliseconds

puts "QSTAT STARTED"

set do_exit 0
log_user 0
while { !$do_exit } {
   set sleep_time [expr [do_qstat] / 1000]
   set timeout $sleep_time
   expect_user {
      "QUIT\n" {
         set do_exit 1
         puts "QSTAT QUIT OK"
      }
      "INTERVAL*\n" {
         set interval [lindex $expect_out(0,string) 1]
         puts "QSTAT INTERVAL OK"
      }
      "START\n" {
         set active 1
         puts "QSTAT START OK"
      }
      "STOP\n" {
         set active 0
         puts "QSTAT STOP OK"
      }
      "*\n" {
         puts "ERROR: invalid input:  $expect_out(0,string)"
      }
   }
}
