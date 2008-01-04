#!/bin/sh
#
#
#
# Sun Grid Engine SMF stop script
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

. /lib/svc/share/smf_include.sh

#Service FMRI we are stopping
service="$1"
#Main contract_id associated with the FMRI
contract_id="$2"
#Currect service state
state="$3"
#Next service state
next_state="$4"


#---------------------------------------------------------------------------
# GetAdminUser
#    echo the name of the admin user on this system
#    echo "root" if admin user retrieval fails
GetAdminUser()
{
   cfgname=$SGE_ROOT/$SGE_CELL/common/bootstrap
   user=none

   if [ -f $cfgname ]; then
      user=`grep admin_user $cfgname | awk '{ print $2 }'`
   fi

   if [ `echo $user|tr "A-Z" "a-z"` = "none" ]; then
      user=root
   fi
   echo $user
}


#---------------------------------------------------------------------------
# MAIN

#echo "   State is $3, next_state: $4"

#We make kill timeout 60sec if not defined
if [ -z "$SGE_SMF_KILL_TIMEOUT" ]; then
   SGE_SMF_KILL_TIMEOUT=60
fi

#TODO: Decouple shephards from execd and use just smf_kill_contract
#We try to kill the contract
#if [ -n "$contract_id" ]; then
#   smf_kill_contract "$contract_id" 15 1 "$SGE_SMF_KILL_TIMEOUT"
#fi

#We try to get the execd pid
if [ -n "$contract_id" ]; then
   pid=`/usr/bin/svcs -l -p "$service" | grep "/sge_execd$" | grep -v "^grep" | awk '{print $2}'`
   #Terminate it
   if [ -n "$pid" ]; then
      echo "Terminating $pid"
      # Kill process
      /usr/bin/kill -15 $pid
      if [ $? -ne 0 ] ; then
         echo "Unable not kill execd that belongs to $1"
         exit 1
      fi

      sleep 1

      ps -p "$pid" > /dev/null 2>&1
      # If process still exists we kill it with kill -9
      if [ "$?" -eq 0 ]; then
         /usr/bin/kill -9 $pid
      fi
   fi
fi

#If we are not at disabled we transition there
if [ "$state" != "disabled" -a "$next_state" != "disabled" ]; then
   /usr/sbin/svcadm disable -t "$service"
fi