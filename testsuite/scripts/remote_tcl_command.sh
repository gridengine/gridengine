#!/bin/sh
# parameter 1: tcl binary with path
# parameter 2: tcl file
# parameter 3: testsuite home directory path
# parameter 4: tcl procedure
# parameter 5: tcl procedure arguments
# parameter 6: testsuite defaults file 
# parameter 7: debug 

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
TCL_BINARY=$1
TCL_FILE=$2
TESTSUITE_ROOT=$3
TCL_PROCEDURE=$4
TCL_ARGS=$5
DEFAULTS_FILE=$6
DO_DEBUG=$7

if [ x$DO_DEBUG = xdebug ]; then
   echo "*** remote_tcl_command.sh start ***"
   echo "tcl binary: $TCL_BINARY"
   echo "tcl file: $TCL_FILE"
   echo "TS root:  $TESTSUITE_ROOT"
   echo "procedure:$TCL_PROCEDURE"
   echo "p-args:   $TCL_ARGS"
   echo "defaults: $DEFAULTS_FILE"
   echo "*** remote_tcl_command.sh end ***"
fi

if [ x$TCL_BINARY = x ]; then
   echo "need tcl binary"
   exit 1
fi

if [ x$TCL_FILE = x ]; then
   echo "need tcl file"
   exit 1
fi
if [ x$TESTSUITE_ROOT = x ]; then
   echo "need testsuite root path"
   exit 1
fi
if [ x$TCL_PROCEDURE = x ]; then
   echo "need tcl procedure name"
   exit 1
fi
if [ x$DEFAULTS_FILE = x ]; then
   echo "need defaults file"
   exit 1
fi



#echo "calling: expect $TCL_FILE $TESTSUITE_ROOT '$TCL_PROCEDURE $TCL_ARGS' no_main file $DEFAULTS_FILE"
$TCL_BINARY $TCL_FILE $TESTSUITE_ROOT "$TCL_PROCEDURE $TCL_ARGS" no_main file $DEFAULTS_FILE quiet 
