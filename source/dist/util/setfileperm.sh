#!/bin/sh
#
# Set file permissions of Grid Engine distribution
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
#
# The $OPTFILES are not mandatory for a distribution and will be set only if
# they exist
#
# This script must be called by user root on a machine where user root has
# permissions to change the ownership of a file
# 
# It is not necessary to run this script if the distribtuon has been
# installed with pkgadd, since pkgadd takes care about the correct
# permissions.
#

PATH=/bin:/usr/bin:/usr/bsd

SECFILELIST="bin lib utilbin"

FILELIST="3rd_party bin ckpt examples install_execd install_qmaster mpi \
          pvm qmon util utilbin"

OPTFILES="catman doc locale man inst_sge inst_sgeee" 

CLIENTFILES="qacct qalter qconf qdel qhost qlogin qmod qmon qrsh qsh \
             qstat qsub"

umask 022

#---------------------------------------------------
# SetFilePerm
#
SetFilePerm()
{
   f="$1"
   user="$2"
   group="$3"

   $INFOTEXT "Verifying and setting file permissions and owner in >%s<" $f

   chmod -R go+r $f
   find $f -type d -exec chmod 755 {} \;
   find $f -type f -perm -100 -exec chmod go+x {} \;
   chown -R $user $f
   chgrp -R $group $f
}

#--------------------------------------------------------------------------
# THE MAIN PROCEDURE
#--------------------------------------------------------------------------

instauto=false
instresport=false

if [ -z "$SGE_ROOT" -o ! -d "$SGE_ROOT" ]; then
   echo 
   echo ERROR: Please set your \$SGE_ROOT environment variable
   echo and restart this script. Exit.
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

#---------------------------------------
# setup INFOTEXT begin
#---------------------------------------

V5BIN=$SGE_ROOT/bin/$ARCH
V5UTILBIN=$SGE_ROOT/utilbin/$ARCH       
INFOTEXT=$V5UTILBIN/infotext
if [ ! -x $INFOTEXT ]; then
   echo "can't find binary \"$INFOTEXT\""
   echo "Installation failed."
   exit 1
fi
SGE_INFOTEXT_MAX_COLUMN=5000; export SGE_INFOTEXT_MAX_COLUMN

#---------------------------------------
# setup INFOTEXT end
#---------------------------------------

if [ $# -lt 3 ]; then
   $INFOTEXT -e "\nSet file permissions and owner of Grid Engine distribution in \$SGE_ROOT\n\n" \
                "Usage: %s [-auto] [-resport] \"adminuser\" \"group\" <sge_root>\n\n" \
                "Example:\n\n" \
                "   # %s sgeadmin adm \$SGE_ROOT\n" `basename $0` `basename $0`
   exit 1
fi

if [ $1 = -auto ]; then
   instauto=true
   shift
fi

if [ $1 = -resport ]; then
   instresport=true
   shift
elif [ $1 = -noresport ]; then
   instresport=false
   shift
fi

if [ $3 = / -o $3 = /etc ]; then
   $INFOTEXT -e "\nERROR: cannot set permissions in \"%s\" directory of your system\n" $3
   exit 1
fi

if [ `echo $3 | env LC_ALL=C cut -c1` != / ]; then
   $INFOTEXT -e "\nERROR: please provide an absolute path for the distribution\n"
   exit 1
fi


if [ $instauto = true ]; then
   :
else
   clear
   $INFOTEXT "\n                    WARNING WARNING WARNING\n" \
             "                    -----------------------\n\n" \
             "We will set the the file ownership and permission to\n\n" \
             "   User:         %s\n" \
             "   Group:        %s\n" \
             "   In directory: %s\n\n" \
             "We will also install the following binaries as SUID-root:\n\n" \
             "   \$SGE_ROOT/utilbin/<arch>/rlogin\n" \
             "   \$SGE_ROOT/utilbin/<arch>/rsh\n" \
             "   \$SGE_ROOT/utilbin/<arch>/testsuidroot\n" $1 $2 $3

   $INFOTEXT -n -ask "yes" "no" \
            "Do you want to set the file permissions (yes/no) [no] >> "

   if [ $? = 1 ]; then
      $INFOTEXT "We will not set the file permissions. Exit."
      exit 1
   fi
fi

cd $3
if [ $? != 0 ]; then
   $INFOTEXT -e "ERROR: can't change to directory \"%s\". Exit." $3
   exit 1
fi

for f in $FILELIST $OPTFILES; do
   if [ -d $f -o -f $f ]; then
      SetFilePerm $f $1 $2
   fi
done

# These files and dirs are owned by root for security reasons
for f in $SECFILELIST; do
   if [ -d $f -o -f $f ]; then
      SetFilePerm $f root root
   fi
done

# Set permissions of SGE_ROOT itself
chown $1 .
chgrp $2 .
chmod 755 .

chown 0 utilbin/*/rsh utilbin/*/rlogin utilbin/*/testsuidroot
chgrp 0 utilbin/*/rsh utilbin/*/rlogin utilbin/*/testsuidroot
chmod 4511 utilbin/*/rsh utilbin/*/rlogin utilbin/*/testsuidroot

if [ $instresport = true ]; then
   for i in $CLIENTFILES; do
      chown 0 bin/*/$i
      chmod 4511 bin/*/$i   
   done
fi

$INFOTEXT "\nYour file permissions were set\n"
