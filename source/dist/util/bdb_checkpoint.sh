#!/bin/sh
#
#  bdb_checkpoint.sh
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
#
##########################################################################
#
# This script is used for transaction checkpointing and cleanup in SGE
# installations with a Berkeley DB RPC server.
#
# It shall be regularly called by the user running berkeley_db_svc.
# It can be called from crontab, e.g. once a minute by the following 
# crontab entry:
# * * * * * <full path to script> <sge root dir> <sge cell> <bdb dir>
#
##########################################################################

# check commandline options
if [ $# -ne 3 ]; then
   echo "usage: $0 <sge_root> <sge_cell> <bdb_home>" >&2
   exit 1
fi

SGE_ROOT=$1
SGE_CELL=$2
BDB_HOME=$3

# check if commandline options are valid
if [ ! -d "${SGE_ROOT}" ]; then
   echo "SGE_ROOT directory ${SGE_ROOT} does not exist" >&2
   exit 1
fi

if [ ! -d "${SGE_ROOT}/${SGE_CELL}" ]; then
   echo "SGE_CELL directory ${SGE_ROOT}/${SGE_CELL} does not exist" >&2
   exit 1
fi

if [ ! -d "${BDB_HOME}" ]; then
   echo "BDB_HOME directory ${BDB_HOME} does not exist" >&2
   exit 1
fi

# get binary architecture
SGE_ARCH=`${SGE_ROOT}/util/arch`

# source settings, we need LD_LIBRARY_PATH/LIBPATH/SHLIB_PATH
. ${SGE_ROOT}/${SGE_CELL}/common/settings.sh

# checkpoint transaction log
result=`${SGE_ROOT}/utilbin/${SGE_ARCH}/db_checkpoint -1 -h ${BDB_HOME} 2>&1`
if [ $? -ne 0 ]; then
   echo "error checkpointing transaction log:" >&2
   echo $result >&2
   exit 1
fi

# retrieve no longer needed transaction logs
logs=`${SGE_ROOT}/utilbin/${SGE_ARCH}/db_archive -h ${BDB_HOME} 2>&1`
if [ $? -ne 0 ]; then
   echo "error retrieving outdated transaction logs:" >&2
   echo $logs >&2
   exit 1
fi

# delete no longer needed transaction logs
for log in $logs; do
   rm -f ${BDB_HOME}/${log}
done

