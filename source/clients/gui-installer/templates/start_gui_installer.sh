#!/bin/sh
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

ErrUsage() 
{
   echo "Usage: `basename $0` [-help] [-resolve_pool=<num>] [-resolve_timeout=<sec>]"
   echo "       [-install_pool=<num>] [-install_timeout=<sec>] [-connect_user=<usr>]"
   echo "       [-connect_mode=windows]"
   echo ""
   echo "   <num> ... decimal number greater than zero"
   echo "   <sec> ... number of seconds, must be greater then zero"
   echo "   <usr> ... user name"
   exit 1
}



FLAGS="-DLOG=true"
ARGUMENTS=""

ARGC=$#
while [ $ARGC != 0 ]; do
   case $1 in
   -debug)
     FLAGS=$FLAGS" -DSTACKTRACE=true"
     DEBUG_ENABLED=true
     ;;
   -help)
     ErrUsage
     ;;
   *)
     ARGUMENTS="$ARGUMENTS $1"
     ;;
   esac
   shift
   ARGC=`expr $ARGC - 1`
done

#Detect JAVA
. ./util/install_modules/inst_qmaster.sh
HaveSuitableJavaBin "1.5.0" "none"
if [ $? -ne 0 ]; then
   echo "Could not find Java 1.5 or later! Install Java from http://www.java.com/getjava and set your JAVA_HOME correcly or put java on your PATH!"
   exit 1
fi

echo "Starting Installer ..."
if [ "$DEBUG_ENABLED" = "true" ]; then
   $java_bin $FLAGS -jar ./util/gui-installer/installer.jar $ARGUMENTS
else
   $java_bin $FLAGS -jar ./util/gui-installer/installer.jar $ARGUMENTS
fi
