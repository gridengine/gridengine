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

PATH=/bin/:/usr/bin

# usage: shutdown_commd.sh -all|host1 host2 ...
#
# Started without argument "-all" shutdown commd's on all given hosts
#
# Started with the "-all" argument shutdown all commd found in the qmaster
# spool directory exec_hosts subdirectory
#

if [ "$SGE_ROOT" = "" ]; then
   echo "SGE_ROOT environment variable not set. Exit."
   exit 1
fi

if [ "$SGE_CELL" = "" ]; then
   SGE_CELL=default
   export $SGE_CELL
fi

if [ $# = 1 -a $1 = "-all" ]; then
   qma_spool_dir=`grep qmaster_spool_dir $SGE_ROOT/$SGE_CELL/common/configuration | \
                  awk '{ print $2 }'`
   targets=`ls $qma_spool_dir/exec_hosts|egrep -v "template|global"`
else
   targets=$*
fi

bin_dir=`grep binary_path $SGE_ROOT/$SGE_CELL/common/configuration | \
              awk '{ print $2 }'`

ARCH=`$SGE_ROOT/util/arch`

if [ ! -x $bin_dir/$ARCH/sgecommdcntl ]; then
   if [ ! -x $bin_dir/sgecommdcntl ]; then
      echo "command >sgecommdcntl< not found in >$bin_dir/$ARCH< or >$bin_dir<. Exit."
      exit 1
   else
      SGECOMMCNTL=$bin_dir/$ARCH/sgecommdcntl
   fi
else
   SGECOMMCNTL=$bin_dir/$ARCH/sgecommdcntl
fi

for h in $targets; do
   echo shutting down commd at host $h
   $SGE_ROOT/bin/$ARCH/sgecommdcntl -k -host $h
done
