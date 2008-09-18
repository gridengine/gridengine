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

#echo "   State is $3, next_state: $4"

#We make kill timeout 60sec if not defined
if [ -z "$SGE_SMF_KILL_TIMEOUT" ]; then
   SGE_SMF_KILL_TIMEOUT=60
fi


#We try to kill the contract
if [ -n "$contract_id" ]; then
   smf_kill_contract "$contract_id" 15 1 "$SGE_SMF_KILL_TIMEOUT"
fi

#If we are not at disabled we transition there
if [ "$state" != "disabled" -a "$next_state" != "disabled" ]; then
   /usr/sbin/svcadm disable -t "$service"
fi