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
#
#

PGHOST=bilbo
DBNAME=sge
export PGHOST

BASEDIR=dist/util/resources/spooling

VERSION="SGEEE pre6.0 (Maintrunk)"

usage()
{
   echo "usage: $0 type action"
   echo "type   = postgres"
   echo "action = on|off"

   exit 1
}

get_history()
{
   history=`psql -Aqtl -c "select with_history from sge_info order by last_change DESC LIMIT 1" $DBNAME`

   if [ $history = t ]; then
      echo 1
   else
      echo 0
   fi
}

enable_history()
{
   psql -c "insert into sge_info values ('now', '$VERSION', true)" $DBNAME
}

disable_history()
{
   psql -f "${BASEDIR}/disable_history.sql" $DBNAME
   psql -c "insert into sge_info values ('now', '$VERSION', false)" $DBNAME
}

# MAIN
if [ $# -lt 2 ]; then
   usage
fi

type=$1
action=$2

# source type specific commands

history=`get_history`

# perform action
case "$action" in 
   on)
      if [ $history -eq 1 ]; then
         echo "history is already on"
         exit 1
      else
         enable_history
      fi
      ;;
   off)
      if [ $history -eq 0 ]; then
         echo "history is already off"
         exit 1
      else
         disable_history
      fi
      ;;
   *)
      echo "unknown action $action"
      echo ""
      usage
      ;;
esac

