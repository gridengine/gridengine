#!/bin/sh
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

#  $ -cwd
#$   -N DEMO 
#$  -S /bin/sh
#  $   -j y

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo ~~~~~~~~~~~~~ Checkpointing-Demonstration~~~~~~~
echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

echo ---
env
echo ---


echo RESTARTED = $RESTARTED

if [ "X$RESTARTED" = "X" ]; then
   RESTARTED=$1
fi

if [ $RESTARTED = 0 ]; then
   echo starting from scratch...
   if [ -f dump.file ]; then
      rm -f dump.file
   fi 
fi

A=0
if [ $RESTARTED != 0 ]; then
   if [ -f dump.file ]; then
      A=`tail -1 dump.file`
      echo "RESTARTED with value $A"
   else  
      echo "RESTARTED without existing dump.file!"
   fi
fi

while [ ! -f ABORT ]; do
   A=`expr $A \+ 1` 
   sleep 1
   echo $A
done

if [ -f ABORT ]; then
   if [ -f MIGRATION ];then 
      echo "Migrating... writing dump file"
      echo $A >> dump.file
      rm -f MIGRATION ABORT
      exit 99
      # triggers restart
   fi
   echo "Aborting... deleting dump.file"
   rm -f ABORT dump.file
fi

