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
#  License at http://www.gridengine.sunsource.net/license.html
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

#
# usage: shutdown_commd.sh -all|host...
#
# Shutdown commd's on all hosts given as argument.
#
# Started without argument "-all" commd's on all hosts 
# found in the directory $SGE_ROOT/default/spool 
# are shutdown.

if [ "x$SGE_ROOT" = "x" ]; then
   echo Assuming /usr/CODINE as \$SGE_ROOT
   SGE_ROOT=/usr/CODINE
   export SGE_ROOT
fi

if [ $# = 1 -a $1 = "-all" ]; then
   targets=`ls $SGE_ROOT/default/spool|egrep -v "qmaster"`
else
   targets=$*
fi

ARCH=`$SGE_ROOT/util/arch`

for h in $targets; do
   echo shutting down commd at host $h
   $SGE_ROOT/bin/$ARCH/sgecommdcntl -k -host $h
done
