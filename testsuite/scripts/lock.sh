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


# $1 lockfile
# $2 pid
# $3 hostname
# $4 user

LOCKFILE=$1
locked=0
loops=0


echo "saving lockfile in $LOCKFILE" 
umask 111
while [ $locked -eq 0 -a $loops -lt 10 ]; do 
  if [ ! -f $LOCKFILE ]; then
     touch $LOCKFILE
     echo "$2 $3 $4" >> $LOCKFILE
     echo "file writen"
     touch $LOCKFILE
     read r_pid r_hostname r_user <$1
     echo "input: $r_pid $r_hostname $r_user"
     if [ x$2 != x -a $2 = $r_pid ]; then
        if [ x$3 != x -a $3 = $r_hostname ]; then
           if [ x$4 != x -a $4 = $r_user ]; then
              locked=10
              break
           fi
        fi
     fi
  fi
  loops=`expr $loops + 1`
  sleep 1
done
umask 022
exit $locked
