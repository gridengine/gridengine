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
qping_binary=$1
host=$2
port=$3
name=$4
id=$5
runs=$6
echo "rerun parameter is $runs"
while [ $runs -gt 0 ]; do 
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  /bin/sh -c "$qping_binary -info $host $port $name $id" &
  output=`/bin/sh -c "$qping_binary -info $host $port $name $id" 2>&1 | grep "timeout"`
  if [ "$output" != "" ]; then
     exit 1
  fi
  wait
  runs=`expr $runs - 1`
done
exit 0


