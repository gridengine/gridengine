#!/bin/sh
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

# The following files and directories will be set by this procedure
#
# The $OPTFILES are not mandatory for a distribution and will be set only if
# they exist
#
# This script must be called by user root on a machine where user root has
# permissions to change the ownership of a file
# 
# It is not necessary to run this script if the distribtuon has been
# installed with pkgadd, since pkgadd takes care about the correct
# permissions

PATH=/bin:/usr/bin

SECFILELIST="bin lib utilbin"

FILELIST="3rd_party bin ckpt examples inst_sge \
install_execd install_qmaster mpi pvm qmon util utilbin"

OPTFILES="catman doc locale man" 

umask 022

#---------------------------------------------------
# SetFilePerm
#
SetFilePerm()
{
   f="$1"
   user="$2"
   group="$3"

   $ECHO "Verifying and setting file permissions and owner in >$f<"

   chmod -R go+r $f
   find $f -type d -exec chmod 755 {} \;
   find $f -type f -perm -100 -exec chmod go+x {} \;
   chown -R $user $f
   chgrp -R $group $f
}

#---------------------------------------------------------------------
# MAIN MAIN
#
if [ -z "$SGE_ROOT" -o ! -d "$SGE_ROOT" ]; then
   echo 
   echo ERROR: Please set your \$SGE_ROOT environment variable correctly
   echo and start this script again.
   echo 
   exit 1
fi

if [ ! -f "$SGE_ROOT/util/arch" ]; then
   echo 
   echo ERROR: The shell script \"$SGE_ROOT/util/arch\" does not exist.
   echo Please verify your distribution and restart this update procedure.
   echo
   exit 1
fi

if [ ! -f $SGE_ROOT/util/arch_variables ]; then
   echo
   echo ERROR: Missing shell script \"$SGE_ROOT/util/arch_variables\".
   echo Please verify your distribution and restart this update procedure.
   echo
   exit 1
fi

. $SGE_ROOT/util/arch_variables

if [ $# != 3 ]; then
   echo
   echo Set file permissions and owner of Grid Engine distribution in \$SGE_ROOT
   echo 
   echo "usage: $0 \"adminuser\" \"group\" <codine_root>"
   echo 
   echo example: $0 codadmin adm 
   echo
   exit 1
fi

if [ $3 = / ]; then
   echo
   echo ERROR: cannot set permissions in root directory of your system.
   echo
   exit 1
fi

if [ `echo $3 | cut -c1` != / ]; then
   echo
   echo ERROR: Please give an absolute path for the distribution.
   echo
   exit 1
fi

clear

$ECHO "                    WARNING WARNING WARNING"
$ECHO "                    -----------------------"
$ECHO "We will set the the file ownership and permission to"
$ECHO
$ECHO "   User:         $1"
$ECHO "   Group:        $2"
$ECHO "   In directory: $3"
$ECHO
$ECHO "We will also install the following binaries as SUID-root:"
$ECHO
$ECHO "   \$SGE_ROOT/utilbin/<arch>/rlogin"
$ECHO "   \$SGE_ROOT/utilbin/<arch>/rsh"
$ECHO "   \$SGE_ROOT/utilbin/<arch>/testsuidroot"
$ECHO

TEXT="Do you want to set the file permissions (yes/no) [NO] >> \c"

YesNo_done=false
while [ $YesNo_done = false ]; do
   $ECHO "$TEXT" 
   read YesNo_INP
   if [ "$YesNo_INP" = "yes" -o "$YesNo_INP" = YES ]; then
      YesNo_done=true
   elif [ "$YesNo_INP" = "NO" -o "$YesNo_INP" = no ]; then
      $ECHO
      $ECHO "We will NOT set the file permissions. Exiting."
      exit 1
   fi
done

cd $3
if [ $? != 0 ]; then
   $ECHO "ERROR: can't change to directory \"$3\". Exiting."
   exit 1
fi

for f in $FILELIST; do
   if [ ! -f $f -a ! -d $f ]; then
      $ECHO
      $ECHO "Obviously this is not a complete SGE distribution or this"
      $ECHO "is not your \$SGE_ROOT directory."
      $ECHO
      $ECHO "Missing file or directory: $f"
      $ECHO
      $ECHO "Your file permissions will not be set. Exiting."
      $ECHO ""
      exit 1
   fi
done

for f in $FILELIST $OPTFILES; do
   if [ -d $f -o -f $f ]; then
      SetFilePerm $f $1 $2
   fi
done

# These files are owned by root for security reasons
for f in $SECFILELIST; do
   if [ -d $f -o -f $f ]; then
      SetFilePerm $f root root
   fi
done

# Set permissions of SGE_ROOT itself
chown $1 .
chgrp $2 .
chmod 755 .

chown $1 bin/*/sgecommdcntl
chmod 550 bin/*/sgecommdcntl

chown 0 utilbin/*/rsh utilbin/*/rlogin utilbin/*/testsuidroot
chgrp 0 utilbin/*/rsh utilbin/*/rlogin utilbin/*/testsuidroot
chmod 4511 utilbin/*/rsh utilbin/*/rlogin utilbin/*/testsuidroot

$ECHO
$ECHO "Your file permissions were set"
$ECHO
