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

#INSTALLDIR=/home/codine/gettextlib/${ARCH}

echo ""
echo "This will configure, compile and install the GNU gettext libary"

set ARCHITECTURE=$1
set INSTALLDIR=$2
set GETTEXTDIR=$3

if ($ARCHITECTURE == "" || $INSTALLDIR == "" || $GETTEXTDIR == "") then
  echo "Error: wrong arguments "
  echo "Usage: ${0} <ARCH> <INSTALL_DIR> <SOURCE_DIR>"
  echo "       <ARCH>        : e.g. SOLARIS64 etc."
  echo "       <INSTALL_DIR> : path where the binaries should be installed"
  echo "                       (e.g. /home/codine/gettext)"
  echo "       <SOURCE_DIR>  : path to the GNU gettext source code"
  exit -1
endif

echo ""
echo "Following parameters are used:"
echo "ARCHITECTURE = ${ARCHITECTURE}"
echo "INSTALLDIR   = ${INSTALLDIR}"
echo "GETTEXTDIR   = ${GETTEXTDIR}"
echo "please wait ..."


#set INSTALLDIR=/home/codine/gettext
#set GETTEXTDIR=/cod_home/crei/src/gettext/gettext-0.10.35
#set ARCHTOOL=/cod_home/crei/src/c4/distrib/dist/util/arch


if ( ${ARCHITECTURE} == "SOLARIS64" ) then
   echo ""
   echo "This is a SOLARIS64 system, we have to do something special ..."
   setenv CFLAGS  "-fast -xarch=v9"
   echo "-end of SOLARIS64 specific options-"
endif

pushd ${GETTEXTDIR}


if (-e Makefile) then
  echo ""
  echo "Cleaning up binaries and object files ..."
  make clean 
   
  echo ""
  echo "Cleaning up old configure files ..."
  make distclean
endif

echo ""
echo "Perform configure (INSTALLDIR is ${INSTALLDIR}) ..."
configure --prefix=${INSTALLDIR} --exec-prefix=${INSTALLDIR}/${ARCHITECTURE} --with-included-gettext --enable-shared

echo ""
echo "Perform make"
make 

echo ""
echo "Perform self-tests"
make check


if !(-e ${INSTALLDIR}) then
  echo ""
  echo "Creating directory ${INSTALLDIR}, chmod to 774"
  mkdir ${INSTALLDIR}
  chmod 774 ${INSTALLDIR}
endif

set OLDFILE=`ls -la ${INSTALLDIR}/${ARCHITECTURE}/bin/gettext`

echo ""
echo "Installing libary into ${INSTALLDIR}"
make install


popd

set NEWFILE=`ls -la ${INSTALLDIR}/${ARCHITECTURE}/bin/gettext`

echo "Old gettext binary file: ${OLDFILE}"
echo "New gettext binary file: ${NEWFILE}"


if ("${OLDFILE}" == "${NEWFILE}") then
   echo ""
   echo "STATE: W A R N I N G : new gettext binary not installed for ${ARCHITECTURE}"
   exit -1
endif

echo ""
echo "STATE: new gettext binary installed for ${ARCHITECTURE}"





