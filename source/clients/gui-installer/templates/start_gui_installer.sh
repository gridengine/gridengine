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
   echo "Usage: `basename $0` [-help] [-resolve_pool=<num>] [-resolve_timeout=<sec>]" \
        "       [-install_pool=<num>] [-install_timeout=<sec>]" \
        "" \
        "   <num> ... decimal number greater than zero" \
        "   <sec> ... number of seconds, must be greater then zero"
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
if [ -n "$JAVA_HOME" -a -f "$JAVA_HOME"/bin/java ]; then
   JAVA_BIN="$JAVA_HOME"/bin/java
else
   JAVA_BIN=`which java`
fi

if [ ! -f "$JAVA_BIN" ]; then
   echo "Java not found! Specify JAVA_HOME or adjust your PATH and try again."
   exit 1
fi

#Try to detect platform
#if [ x`./util/arch | grep 64` != x ]; then
#   FLAGS="-d64 $FLAGS"
#fi
echo "Starting Installer ..."
if [ "$DEBUG_ENABLED" = "true" ]; then
   $JAVA_BIN $FLAGS -jar ./util/gui-installer/installer.jar $ARGUMENTS
else
   $JAVA_BIN $FLAGS -jar ./util/gui-installer/installer.jar $ARGUMENTS
fi