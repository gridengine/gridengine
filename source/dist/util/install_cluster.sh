#!/bin/sh
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

PATH=/bin:/usr/bin

#
# Install execution daemons on a given list of hosts via rsh or ssh access
#
# "$SGE_ROOT" must be set in current environment.
# 
# $1 may be the string "-noqueue"
# this parameter will be passed to the "install_execd" script on the remote
# machine.
#

if [ "$SGE_ROOT" = "" ]; then
   echo environment varibale \"\$SGE_ROOT\" not set - installation failed
   exit 1
fi


if [ $# -lt 1 -o "$1" = "-h" -o "$1" = "-help" ]; then
   echo "install execution daemons via \"rsh\" or \"ssh\" access "
   echo
   echo "usage: $0 [-noqueue] [-ssh] host1 host2 host3 ..."
   echo "       -ssh      use ssh command instead of rsh"
   echo "       -noqueue  do not add ad default queue when installing the exec host"
   echo 
   echo "The following command will be executed"
   echo
   echo "   # rsh|ssh <hostname> \"cd $SGE_ROOT && ./install_execd -fast -auto [-noqueue]\""
   exit 1
fi


if [ "$1" = "-noqueue" ]; then
   noqueue=-noqueue
   shift
else
   noqueue=""
fi

if [ $# -lt 1 ]; then
   echo please give a list of hosts in command line where to install execution daemons
   exit 1
fi

if [ "$SGE_CELL" = "" ]; then
   SGE_CELL=default
   export SGE_CELL
fi

echo Installing execution hosts for cell \"$SGE_CELL\"

for host in $*; do
   echo
   echo Installing execution daemon on host \"$host\"

   echo ". $SGE_ROOT/$SGE_CELL/common/settings.sh; cd $SGE_ROOT && ./install_execd -fast -auto $noqueue" | rsh $host /bin/sh
   echo ==============================================================================
done
