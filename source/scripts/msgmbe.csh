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

set ARCH = `./aimk -nomk`
set MBEMSG = /usr/bin/mbemsg
set MSGFMT = ./3rdparty/gettext-0.11/${ARCH}/bin/msgfmt
set LOCALEDIR = "./dist/locale"
set MSGPO = "gridengine.po"
set MSGMO = "gridengine.mo"
set MSGPOT = "gridengine.pot"



if ( ! -f ${MBEMSG} ) then
  echo "${MBEMSG} not found."
  exit -1
endif


if ( ! -f ${MSGFMT} ) then
  echo "${MBEMSG} not found."
  exit -1
endif

if ( ! -d ${LOCALEDIR}/en_FW.MBE/LC_MESSAGES ) then
      mkdir -p ${LOCALEDIR}/en_FW.MBE/LC_MESSAGES
endif

if ( ! -d ${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES ) then
      mkdir -p ${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES
endif



unsetenv LANG
echo "generating ascii gridengine.po file in ${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES/ ... "
$MBEMSG -o ${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES/gridengine.po -g ${LOCALEDIR}/${MSGPOT}
echo "done. "

setenv LANG en_FW.MBE
echo "generating MBE gridengine.po file in ${LOCALEDIR}/en_FW.MBE/LC_MESSAGES/ ... "
$MBEMSG -o ${LOCALEDIR}/en_FW.MBE/LC_MESSAGES/gridengine.po -g ${LOCALEDIR}/${MSGPOT}
echo "done. "


echo "generating binary message file gridengine.mo in ${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES/ ... "
$MSGFMT --output-file=${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES/gridengine.mo ${LOCALEDIR}/en_FW.ASCII/LC_MESSAGES/gridengine.po
echo "done."

echo "generating binary message file gridengine.mo in ${LOCALEDIR}/en_FW.MBE/LC_MESSAGES/ ... "
$MSGFMT --output-file=${LOCALEDIR}/en_FW.MBE/LC_MESSAGES/gridengine.mo ${LOCALEDIR}/en_FW.MBE/LC_MESSAGES/gridengine.po 
echo "done."



 
