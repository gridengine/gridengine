#!/bin/sh
#
# Install system wide Grid Engine startup script
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
# set -x   

# Reset PATH to a safe value
#
PATH=/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb
umask 022
  
# Easy way to prevent clearing of screen
#
CLEAR=clear
#CLEAR=:

# set to false if you don't want this script uses the i18n functions
#
SGE_I18N=true

#--------------------------------------------------------------------------
# THE MAIN PROCEDURE
#--------------------------------------------------------------------------

#----------------------------------
# GET ARCH + ARCH SPECIFIC DEFAULTS
#

umask 022

if [ -z "$SGE_ROOT" -o ! -d "$SGE_ROOT" ]; then
   echo
   echo ERROR: Please set your \$SGE_ROOT environment variable
   echo and start this script again. Exit.
   echo
   exit 1
fi

if [ ! -f "$SGE_ROOT/util/arch" ]; then
   echo
   echo ERROR: The shell script \"$SGE_ROOT/util/arch\" does not exist.
   echo Please verify your distribution and restart this script. Exit.
   echo
   exit 1
fi

if [ ! -f $SGE_ROOT/util/arch_variables ]; then
   echo
   echo ERROR: Missing shell script \"$SGE_ROOT/util/arch_variables\".
   echo Please verify your distribution and restart this script. Exit.
   echo
   exit 1
fi


. $SGE_ROOT/util/arch_variables

if [ "$SGE_CELL" = "" ]; then
   SGE_CELL=default
fi

#---------------------------------------
# setup INFOTEXT begin
#---------------------------------------

V5BIN=$SGE_ROOT/bin/$ARCH
V5UTILBIN=$SGE_ROOT/utilbin/$ARCH
INFOTEXT=$V5UTILBIN/infotext
if [ ! -x $INFOTEXT ]; then
   echo "can't find binary \"$INFOTXT\""
   echo "Installation failed."
   exit 1
fi
SGE_INFOTEXT_MAX_COLUMN=5000; export SGE_INFOTEXT_MAX_COLUMN

#---------------------------------------
# setup INFOTEXT end
#---------------------------------------


ME=`whoami`
if [ "$ME" = "" ]; then
   $INFOTEXT -e "Can't determine your username with \"whoami\" command. Exit."
   exit 1
fi

if [ "$ME" != root ]; then
   $INFOTEXT -e "Need to be user >root< to install startup script."
   exit 1
fi

STARTUP_FILE_NAME=rcsge
S95NAME=S95rcsge

if [ ! -f $SGE_ROOT/$SGE_CELL/common/$STARTUP_FILE_NAME ]; then
   $INFOTEXT -e "Can't find startup script: %s" $SGE_ROOT/$SGE_CELL/common/$STARTUP_FILE_NAME
   exit 1
fi

if [ "$RC_FILE" = "sysv_rc" ]; then
   $INFOTEXT "Installing startup script: %s" "$RC_PREFIX/$RC_DIR/$S95NAME"

   if [ -f $RC_PREFIX/$RC_DIR/S95codine5 ]; then
      $INFOTEXT "   Deleting old startup script: %s" $RC_PREFIX/$RC_DIR/S95codine5
      rm $RC_PREFIX/$RC_DIR/S95codine5
   fi

   if [ -f $RC_PREFIX/$RC_DIR/S95grd5 ]; then
      $INFOTEXT "   Deleting old startup script: %s" $RC_PREFIX/$RC_DIR/S95grd5
      rm $RC_PREFIX/$RC_DIR/S95grd5
   fi

   Execute rm -f $RC_PREFIX/$RC_DIR/$S95NAME
   Execute cp $SGE_ROOT/$SGE_CELL/common/$STARTUP_FILE_NAME $RC_PREFIX/init.d/$STARTUP_FILE_NAME
   Execute chmod a+x $RC_PREFIX/init.d/$STARTUP_FILE_NAME
   Execute ln -s $RC_PREFIX/init.d/$STARTUP_FILE_NAME $RC_PREFIX/$RC_DIR/$S95NAME

   # runlevel management in Linux is different -
   # each runlevel contains full set of links
   # RedHat uses runlevel 5 and SUSE runlevel 3 for xdm
   # RedHat uses runlevel 3 for full networked mode
   # Suse uses runlevel 2 for full networked mode
   # we already installed the script in level 3
   if [ $ARCH = glinux -o $ARCH = alinux -o $ARCH = slinux ]; then
      runlevel=`grep "^id:.:initdefault:"  /etc/inittab | cut -f2 -d:`
      if [ "$runlevel" = 2 -o  "$runlevel" = 5 ]; then
         $INFOTEXT "Installing startup script also in %s" "$RC_PREFIX/rc${runlevel}.d/$S95NAME"
         Execute rm -f $RC_PREFIX/rc${runlevel}.d/$S95NAME
         Execute ln -s $RC_PREFIX/init.d/$STARTUP_FILE_NAME $RC_PREFIX/rc${runlevel}.d/$S95NAME
      fi
   fi
elif [ "$RC_FILE" = "insserv-linux" ]; then
   $INFOTEXT "Installing startup script >%s< with >%s<" $RC_PREFIX/$STARTUP_FILE_NAME insserv
   Execute cp $SGE_ROOT/$SGE_CELL/common/$STARTUP_FILE_NAME $RC_PREFIX/$STARTUP_FILE_NAME
   /sbin/insserv $RC_PREFIX/$STARTUP_FILE_NAME
else
   $INFOTEXT "Adding the startup script to your boot scripts is not supported\n"
             "on this operating system."
fi
