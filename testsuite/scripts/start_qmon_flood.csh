#/bin/csh
#
#
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

echo "press enter to start and another enter to kill all qmons after startup"
$<

echo "" > pidfile.txt
set pidlist = ""
set counter = 1400
set waitcount = 200
setenv SGE_QMON_TEST_QMASTER_ISALIVE 10
while ( $counter >= 1 ) 
   echo "to start: $counter"
   set counter = `expr $counter - 1`
   set waitcount = `expr $waitcount - 1`
   echo "waitcount=$waitcount"
   qmon &
   set pid = $! 
   echo "$pid" >> pidfile.txt
   if ( $waitcount == 0 ) then
      echo "sleeping"
      sleep 2
      set waitcount = 200
   endif
end 

echo "press enter to kill all started qmons"
$<

set waitcount = 500
foreach pid ( `cat pidfile.txt` )
echo "killing pid $pid ..."
kill $pid
set waitcount = `expr $waitcount - 1`
if ( $waitcount == 0 ) then
      echo "sleeping"
      sleep 1
      set waitcount = 500
   endif
end

wait

rm pidfile.txt

