#!/bin/csh -f
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

set exec = "qsub ${GRD_ROOT}/examples/jobs/sleeper.sh"
echo ${exec}
set input = $<
$exec
$exec
$exec
$exec
$exec
$exec
$exec
$exec
$exec
$exec
echo "********************"

echo "qconf -sc host:"
set input = $<
qconf -sc host
echo "********************"

echo "qconf -acal testcalendar"
set input = $<
qconf -acal testcalendar
echo "********************"

echo "qconf -scal testcalendar"
set input = $<
qconf -scal testcalendar
echo "********************"

echo "qconf -scall"
set input = $<
qconf -scall
echo "********************"

echo "qconf -ackpt testckpt"
set input = $<
qconf -ackpt testckpt
echo "********************"

echo "qconf -sckpt testckpt"
set input = $<
qconf -sckpt testckpt
echo "********************"

echo "qconf -sckptl"
set input = $<
qconf -sckptl
echo "********************"

echo "qconf -scl"
set input = $<
qconf -scl
echo "********************"

echo "qconf -sconf"
set input = $<
qconf -sconf 
echo "********************"

echo "qconf -sconfl"
set input = $<
qconf -sconfl
echo "********************"

echo "qconf -se dwain"
set input = $<
qconf -se dwain
echo "********************"

echo "qconf -sel"
set input = $<
qconf -sel
echo "********************"

echo "qconf -sep"
set input = $<
qconf -sep
echo "********************"

echo "qconf -sh"
set input = $<
qconf -sh
echo "********************"

echo "qconf -sm"
set input = $<
qconf -sm
echo "********************"

echo "qconf -so"
set input = $<
qconf -so
echo "********************"

echo "qconf -ap testparallel"
set input = $<
qconf -ap testparallel
echo "********************"

echo "qconf -sp testparallel"
set input = $<
qconf -sp testparallel
echo "********************"

echo "qconf -spl"
set input = $<
qconf -spl
echo "********************"

echo "qconf -sq DWAIN.q"
set input = $<
qconf -sq DWAIN.q
echo "********************"

echo "qconf -sql"
set input = $<
qconf -sql
echo "********************"

echo "qconf -ss"
set input = $<
qconf -ss
echo "********************"

echo "qconf -sss"
set input = $<
qconf -sss
echo "********************"

echo "qconf -ssconf"
set input = $<
qconf -ssconf
echo "********************"

echo "qconf -sstnode testnode"
set input = $<
qconf -sstnode testnode
echo "********************"

echo "qconf -sstree"
set input = $<
qconf -sstree
echo "********************"

echo "qconf -su defaultdepartment"
set input = $<
qconf -su defaultdepartment
echo "********************"

echo "qconf -aumap crei"
set input = $<
qconf -aumap crei
echo "********************"

echo "qconf -sumap crei"
set input = $<
qconf -sumap crei
echo "********************"

echo "qconf -sumapl"
set input = $<
qconf -sumapl
echo "********************"

echo "qconf -auser"
set input = $<
qconf -auser testuser
echo "********************"

echo "qconf -suser testuser"
set input = $<
qconf -suser testuser
echo "********************"

echo "qconf -aprj"
set input = $<
qconf -aprj testprj
echo "********************"

echo "qconf -sprj testprj"
set input = $<
qconf -sprj testprj
echo "********************"

echo "qconf -sul"
set input = $<
qconf -sul
echo "********************"

echo "qconf -suserl"
set input = $<
qconf -suserl
echo "********************"




set exec = "qacct -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qacct"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qacct -h bolek"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qacct -g codine"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qacct -o crei"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qalter -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qconf -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qdel -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qdel -verify all"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qdel -uall"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qsub ${GRD_ROOT}/examples/jobs/sleeper.sh"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qdel -uall"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qhold -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qhost -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qhost"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qhost -u crei"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qlogin -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qlogin"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -c BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -d BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -e BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -s BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -us BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmon -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmon &"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qrexec -help"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qrls -help"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qrsh -help"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qrsh"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qmod -c BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"



set exec = "qselect -help"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qselect -l arch=solaris64 "
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qsh -help"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qsh"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qstat -help"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qstat -f"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qstat -f -alarm -ext"
echo ${exec}
set input = $<
$exec
echo "********************"


set exec = "qmod -c BOLEK.q BALROG.q DWAIN.q FANGORN.q"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qsub -help"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qsub -t 10:100:10 ${GRD_ROOT}/examples/jobs/sleeper.sh"
echo ${exec}
set input = $<
$exec
echo "********************"

set exec = "qstat -f -alarm -ext"
echo ${exec}
set input = $<
$exec
echo "********************"

