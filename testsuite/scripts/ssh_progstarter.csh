#!/bin/csh

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
echo "ssh_progstarter.csh:"
echo "********************"
echo "workdir:          =$1="
echo "SGE_ROOT:         =$2="
echo "COMMD_PORT:       =$3="
echo "SGE_QMASTER_PORT: =$3="
set execd_port=`expr ${3} + 1`
echo "SGE_EXECD_PORT:   =${execd_port}="
echo "run command:      =$4="
echo "arg(5)            =$5="
echo "arg(6)            =$6="
echo "arg(7)            =$7="
echo "arg(8)            =$8="
echo "arg(9)            =$9="

set mycom = $4
cd $1
setenv SGE_ROOT $2
setenv COMMD_PORT $3
setenv SGE_QMASTER_PORT $3
setenv SGE_EXECD_PORT $execd_port

set counter = 5
set com_args = ""
while ( $counter <= $#argv ) 
  set argument = "$argv[$counter]"
  set counter = `expr $counter + 1`

  if ( $counter == 6 ) then
     set com_args = "${com_args}"\""$argument"\"
  endif

  if ( $counter != 6 ) then
     set com_args = "${com_args}"' '\""$argument"\"
  endif
end

echo "running: $mycom $com_args"
eval exec $mycom $com_args
