#!/bin/sh
#
# Update CODINE/GRD/SGE(EE) file to version 5.3
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
PATH=/bin:/usr/bin:/usr/ucb:/sbin:/usr/sbin
  
# Easy way to prevent clearing of screen
#
CLEAR=clear
#CLEAR=:

# set to false if you don't want this script uses the i18n functions
#
SGE_I18N=true

#-------------------------------------------------------------------------
# USEFUL LOCAL SHELL PROCEDURES

# --------------------------------------------------------------------------
# Check for old jobs in qmaster spool directory and
# exit if there are any
#
CheckForOldJobs()
{
   first=true
   doexit=false
   for i in `ls $MSPOOLdir/jobs` ; do
         if [ $first = true ]; then
            echo
            echo "           CRITICAL WARNING"
            echo "           ----------------"
            echo 
            echo "There are old jobs in the directory"
            echo 
            echo "   $MSPOOLdir/jobs"
            echo
            echo "The following jobs must be deleted manually before continuing"
            echo "with this update procedure."
            echo
            $ECHO "Press <RETURN> to see the job numbers >> \c"
            read dummy
            first=false
            doexit=true
         fi
         $ECHO "`basename $i` \c"
   done

   first=true
   for i in `ls $MSPOOLdir/job_scripts` ; do
      if [ $first = true ]; then
         echo
         echo
         echo "           CRITICAL WARNING"
         echo "           ----------------"
         echo 
         echo "There are old job scripts in the directory"
         echo
         echo "     $MSPOOLdir/job_scripts"
         echo 
         echo "The following job scripts must be deleted manually before continuing"
         echo "with this update procedure."
         echo
         $ECHO "Press <RETURN> to see the job script numbers >> \c"
         read dummy 
         first=false
         doexit=true
      fi
      $ECHO "`basename $i` \c"
   done

   if [ $doexit = true ]; then
      $ECHO "\n\nPress <RETURN> to continue >> \c"
      read dummy
      clear
      echo
      echo Please delete the files in your \"jobs/\" and \"job_scripts\" directories
      echo before restarting this update procedure.
      echo
      echo Please verify if the spool directories of your execution daemons
      echo do not contain any old jobs.
      echo 
      echo You can delete the following directories of your 
      echo execution host spool directories:
      echo
      echo "       active_jobs/"
      echo "       jobs/"
      echo "       job_scripts/"
      echo 
      exit 1
   fi
}

#--------------------------------------------------------------------------
# CheckWhoCalls this script
#   check if admin user is used and if caller of the script is adminuser
#   exit if this is not true
#
CheckWhoCalls()
{
   if [ "$ADMINUSER" != default -a "$ME" != $ADMINUSER ]; then
      $CLEAR
      $ECHO
      Translate 0 "This is an adminuser installation"
      $ECHO "---------------------------------"
      $ECHO
      Translate 0 "According to your global cluster configuration your Grid Engine"
      Translate 0 "installation used the adminuser name"
      $ECHO
      Translate 0 "   >%s<" $ADMINUSER
      $ECHO
      Translate 0 "to create its spool files. Please login as user >%s<" $ADMINUSER
      Translate 0 "and restart this update script."
      $ECHO
      Translate 0 "Exiting update procedure. No changes were made."
      $ECHO
      exit 1
   fi
}

#--------------------------------------------------------------------------
# WelcomeTheUser   
#
WelcomeTheUser()
{

   $CLEAR

   $ECHO
   Translate 0 "Updating Grid Engine configuration to version 5.3"
   $ECHO "-------------------------------------------------"
   Translate 0 ""
   Translate 0 "The environment variables"
   Translate 0 ""
   Translate 0 "   SGE_ROOT"
   Translate 0 "   SGE_CELL     (only if other than \"default\")"
   Translate 0 ""
   Translate 0 "must be set and point to the directory of your Grid Engine installation."
   Translate 0 ""
   Translate 0 "You may not use this script unless you are upgrading from:"
   $ECHO
   Translate 0 "   CODINE versions 5.0.x and 5.1.x"
   Translate 0 "   GRD versions 5.0.x and 5.1.x"
   Translate 0 "   SGE 5.2.x"
   Translate 0 "   SGE(EE) 5.3beta1"
   $ECHO
   Translate 0 "Please read the update documentation file:"
   $ECHO
   Translate 0 "   %s" \$SGE_ROOT/UPGRADE
   $ECHO
   Translate 0 "before starting this update procedure."

   Translate 4 "Do you want to proceed with the update proecdure"
   YesNo "\n$transout" n

   if [ $? = 1 ]; then
      $ECHO
      Translate 0 "Exiting update procedure. No changes were made."
      exit 1
   fi
}

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

#-------------------------------------------------------------------------
# setup i18n

if [ "$GETTEXT" != "" -a "SGE_I18N" = true ]; then
   unset TEXTDOMAINDIR TEXTDOMAIN
   TEXTDOMAINDIR="`/bin/pwd/`locale"
   TEXTDOMAIN=gridengine
   translation=1
else
   translation=0
   unset LANG LC_ALL LC_COLLATE LC_CTYPE LC_MESSAGES LC_MONETARY
   unset LC_NUMERIC LC_TIME LANGUAGE
fi

# end of internationalization setup
#-------------------------------------------------------------------------

ME=`whoami`
if [ "$ME" = "" ]; then
   Translate 0 "Can't determine your username with \"%s\" command. Exit." whoami
   exit 1
fi

# Set variable $ADMINUSER
#
SetAdminUser

V5BIN=$SGE_ROOT/bin/$ARCH
V5UTILBIN=$SGE_ROOT/utilbin/$ARCH

WelcomeTheUser
CheckWhoCalls


C_DIR=$SGE_ROOT/$SGE_CELL/common
CMD_DIR=$SGE_ROOT/util/update_commands


if [ ! -d "$C_DIR" ]; then
   echo 
   echo "ERROR: The directory"
   echo ""
   echo "     $C_DIR"
   echo ""
   echo "does not exist. Please verify you environment variables and start"
   echo "this update script again."
   echo
   exit 1
fi


if [ ! -f $C_DIR/configuration ]; then
   echo
   echo "ERROR: The cluster configuration file"
   echo ""
   echo "   $C_DIR/configuration"
   echo ""
   echo "does not exist. Please verify your environment variables and start"
   echo "this update script again."
   echo
   exit 1
fi

if [ ! -f $SGE_ROOT/util/startup_template ]; then
   echo
   echo "ERROR: The new $QSYST startup template file"
   echo ""
   echo "   $SGE_ROOT/util/startup_template"
   echo ""
   echo "does not exist. Please verify your distribution"
   echo "and start this update script again."
   echo
   exit 1
fi


MSPOOLdir=`grep qmaster_spool_dir $C_DIR/configuration | awk '{print $2}'`

if [ ! -d "$MSPOOLdir" ]; then
   echo
   echo "ERROR: Apparently your qmaster spool directory"
   echo ""
   echo "     $MSPOOLdir"
   echo ""
   echo "which is defined in the file"
   echo ""
   echo "     $C_DIR/configuration"
   echo ""
   echo "does not exist. Please verify your configuration and start"
   echo "this update script again."
   echo
   exit 1
else
   clear
   echo
   echo "$QSYST qmaster spool directory"
   echo "$LINES------------------------"
   echo
   echo Using directory
   echo
   echo "     $MSPOOLdir"
   echo
   echo "from your cluster configuration as qmaster spool directory for this"
   echo "update procedure."
   echo
   $ECHO "Do you want to continue and use this directory (y/n) [N] >> \c"
   read yes
   if [ "$yes" = y -o "$yes" = Y ]; then
      : 
   else   
      echo
      echo Exiting update procedure. Please verify your cluster configuration file and
      echo restart this update procedure.
      echo
      exit 1 
   fi
fi


#-----------------------------------------------------------------
clear
echo

if [ -f $SGE_ROOT/doc/CODINE5_0.ps -o -f $SGE_ROOT/doc/GRD5_0.ps ]; then
   OLDVERSION=5.0
   NOTOLDVERSION=${VERSION}.2
else
   OLDVERSION=${VERSION}.2
   NOTOLDVERSION=5.0
fi

echo
echo "Guessing your previous $QSYST version"
echo "-----------------------$LINES--------"
echo
echo "It seems that your old $QSYST version is $OLDVERSION."
echo
echo "You can run this update procedure either from version ${VERSION}.2 or 5.0."
echo ""
echo "If you are upgrading from version 5.0 no configuration files will be changed"
echo "and only some old files will be deleted and a new startup script is created."
echo
$ECHO "Is your previous $QSYST system version $OLDVERSION (y/n) [Y] >> \c"
read yes

echo

if [ "$yes" = "" -o "$yes" = y -o "$yes" = Y ]; then
   $ECHO "This script will update your system from version $OLDVERSION --> 5.1"
else
   $ECHO "This script will update your system from version $NOTOLDVERSION --> 5.1"
   OLDVERSION=5.0
fi

echo
$ECHO "Hit <RETURN> to begin or press INTR (Ctrl-C) to stop this procedure >> \c"
read ans

#-----------------------------------------------------------------
# IMPORTANT CHECK/1 - not from 5.0
# check if this update procedure already ran

if [ $OLDVERSION != 5.0 ]; then 
   OLD_QUEUEDIR=queues-${VERSION}.2
   OLD_EXECDIR=exec_hosts-${VERSION}.2
   OLD_PEDIR=pe-${VERSION}.2
   OLD_ACCFILE=accounting-${VERSION}.2
   OLD_CONFFILE=configuration-${VERSION}.2
   OLD_SCHEDCONFFILE=sched_configuration-${VERSION}.2
   OLD_HOSTCOMPLEX=host_complex-${VERSION}.2

   found=false
   for i in $MSPOOLdir/$OLD_QUEUEDIR $MSPOOLdir/$OLD_EXECDIR $MSPOOLdir/$OLD_PEDIR; do
      if [ -d $i ]; then
         found=true
         echo Directory already exists: $i
      fi
   done

   for i in $C_DIR/$OLD_ACCFILE $C_DIR/$OLD_CONFFILE $C_DIR/$OLD_SCHEDCONFFILE $MSPOOLdir/$OLD_HOSTCOMPLEX; do
      if [ -f $i ]; then
         found=true
         echo File already exists: $i
      fi
   done


   if [ $found = true ]; then
      echo 
      echo "Apparently you already run this update procedure."
      echo 
      echo "Please rename the listed backup directory(s) and file(s) and restart"
      echo "this update procedure."
      echo
      exit 1
   fi
fi
    
#-----------------------------------------------------------------
# IMPORTANT CHECK/2
# check if there are old jobs in the qmaster spool directory and exit if yes
#
CheckForOldJobs

clear
echo
echo Starting update $QSYST procedure
echo ----------------$LINES----------
echo
echo "Deleting $QSYST version $VERSION.0, $VERSION.1 $VERSION.2 files and directories"
echo

#-----------------------------------------------------------------
# DELETE: cleanup of CODINE 4.0 files
#
if [ -f $C_DIR/schedd_configuration ]; then
   echo Deleting $QSYST $VERSION.0 \"$C_DIR/schedd_configuration\" file
   Execute rm $C_DIR/schedd_configuration
fi   

if [ -f $C_DIR/schedd_configuration.obsolete ]; then
   echo Deleting $QSYST $VERSION.0 scheduler configuration backup file
   Execute rm $SGE_ROOT/$SGE_CELL/common/schedd_configuration.obsolete
fi

if [ -f $C_DIR/configuration.old ]; then
   echo Deleting $QSYST $VERSION.0 backup cluster configuration file \"$C_DIR/configuration.old\" 
   Execute rm $C_DIR/configuration.old
fi

if [ -f $SGE_ROOT/util/conv_exec.sh ]; then
   echo Deleting $QSYST $VERSION.0 update procedure \"$SGE_ROOT/util/conv_exec.sh\"
   Execute rm $SGE_ROOT/util/conv_exec.sh
fi

if [ -f $SGE_ROOT/util/update_4.01-4.02.sh ]; then
   echo Deleting $QSYST $VERSION.0 update procedure \"$SGE_ROOT/util/update_4.01-4.02.sh\"
   Execute rm $SGE_ROOT/util/update_4.01-4.02.sh
fi

if [ -f $SGE_ROOT/util/defmanpath ]; then
   echo Deleting $QSYST $VERSION.0 script \"$C_DIR/defmanpath\"
   Execute rm $SGE_ROOT/util/defmanpath
fi

if [ -f $MSPOOLdir/old_host_complex ]; then
   echo Deleting $QSYST $VERSION.0 back host complex file \"$MSPOOLdir/old_host_complex\"
   Execute rm $MSPOOLdir/old_host_complex
fi

if [ -d $MSPOOLdir/queues4.02b ]; then
   echo Deleting backup queue directory of $QSYST $VERSION.0
   Execute rm -rf $MSPOOLdir/queues4.02b 
fi


#-----------------------------------------------------------------
# DELETE: cleanup of CODINE 4.1 files
#
if [ -f $SGE_ROOT/README_4.02-4.1 ]; then
   echo Deleting $QSYST $VERSION.1 readme file \"$SGE_ROOT/README_4.02-4.1\"
   Execute rm $SGE_ROOT/README_4.02-4.1
fi

if [ -f $SGE_ROOT/util/update_4.02-4.1.sh ]; then
   echo Deleting $QSYST $VERSION.1 update script \"$SGE_ROOT/util/update_4.02-4.1.sh\2
   Execute rm $SGE_ROOT/util/update_4.02-4.1.sh
fi

if [ -f $C_DIR/gds_mode ]; then
   echo Deleting \"$C_DIR/gds_mode\" file
   Execute rm $C_DIR/gds_mode
fi

if [ -f $C_DIR/configuration.41 ]; then
   echo Deleting $QSYST $VERSION.1 global cluster configuration file
   Execute rm $C_DIR/configuration.41
fi

#-----------------------------------------------------------------
# DELETE: cleanup of x.2 files
#
echo Deleting $QSYST VERSION_${VERSION}\* files
rm -f $SGE_ROOT/VERSION_${VERSION}*

if [ -f $SGE_ROOT/README-${VERSION}2 ]; then
   echo Deleting $QSYST $VERSION.2 readme file \"$SGE_ROOT/README_-${VERSION}2\"
   Execute rm $SGE_ROOT/README-${VERSION}2
fi

echo Deleting $QSYST $VERSION.2 files in \"$SGE_ROOT/util\"
if [ -d $SGE_ROOT/util/sedcommands42 ]; then
   Execute rm -rf $SGE_ROOT/util/sedcommands42
fi

rm -f $SGE_ROOT/util/cod_update-42.sh
rm -f $SGE_ROOT/util/codine4_startup_template
rm -f $SGE_ROOT/util/codine_arch
rm -f $SGE_ROOT/util/complexes-42.sh

if [ $mode = codine ]; then
   rm -f $SGE_ROOT/util/cod_update-42.sh
else
   rm -f $SGE_ROOT/util/grd_update-12.sh
   rm -f $SGE_ROOT/util/codine_aliases
fi

#-----------------------------------------------------------------
# DELETE: cleanup of CODINE 5.0 files
#
echo Deleting $QSYST VERSION_5.0\* files
rm -f $SGE_ROOT/VERSION_5.0*
rm -rf $SGE_ROOT/util/commands50

if [ $mode = codine ]; then
   rm -f $SGE_ROOT/util/cod_update-50.sh
else
   rm -f $SGE_ROOT/util/grd_update-50.sh
fi

#-----------------------------------------------------------------
# DELETE: history directory tree - not from 5.0
#
if [ $OLDVERSION != 5.0 ]; then
   if [ -d $C_DIR/history ]; then
      echo Deleting \"$C_DIR/history\" directory tree
      Execute rm -rf $C_DIR/history
      Execute mkdir $C_DIR/history
   fi
fi

#-----------------------------------------------------------------
# DELETE: statistics file
#
if [ -f $C_DIR/statistics ]; then
   echo Deleting \"$C_DIR/statistics\" file
   Execute rm $C_DIR/statistics
fi        

#-----------------------------------------------------------------
# UPGRADE: global cluster configuration - not from 5.0
#
if [ $OLDVERSION != 5.0 ]; then
   echo Updating global cluster config file \"$C_DIR/configuration\"
   Execute mv $C_DIR/configuration $C_DIR/$OLD_CONFFILE
   $CMD_DIR/configuration2-51.sh -$mode $C_DIR/$OLD_CONFFILE $C_DIR/configuration $QLOGIN_DAEMON 30000-30100
fi

#-----------------------------------------------------------------
# UPGRADE: accounting file - not from 5.0
#
if [ $OLDVERSION != 5.0 ]; then
   if [ -f $C_DIR/accounting ]; then
      echo Updating accounting file
      Execute mv $C_DIR/accounting $C_DIR/$OLD_ACCFILE
      $AWK -v mode=$mode -f $CMD_DIR/accounting2-51.awk $C_DIR/$OLD_ACCFILE > $C_DIR/accounting
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: scheduler configuration - not from 5.0
#
if [ $OLDVERSION != 5.0 ]; then
   if [ -f $C_DIR/sched_configuration ]; then
      echo Updating scheduler configuration
      Execute cp $C_DIR/sched_configuration $C_DIR/$OLD_SCHEDCONFFILE
      grep "^schedd_job_info" $C_DIR/sched_configuration 2>&1 > /dev/null
      status=$?
      if [ $status != 0 ]; then
         echo "schedd_job_info            true" >> $C_DIR/sched_configuration
      fi
   fi
fi

echo
echo "Finished upgrading files in \"$C_DIR\""
echo 
$ECHO "Press <RETURN> to continue >> \c"
read dummy

# No configuration files format changed since 5.0
#
if [ $OLDVERSION != 5.0 ]; then

   # UPGRADE: "queues"
   #
   if [ ! -d $MSPOOLdir/queues ]; then
      echo 
      echo ERROR: Cannot find directory: $MSPOOLdir/queues
      echo Update failed.
      echo
      exit 1
   else
      echo
      echo Updating queues. Your old queues are saved in 
      echo
      echo "   $MSPOOLdir/$OLD_QUEUEDIR"
      echo 

      Execute mv $MSPOOLdir/queues $MSPOOLdir/$OLD_QUEUEDIR
      Execute mkdir $MSPOOLdir/queues

      for i in `ls $MSPOOLdir/$OLD_QUEUEDIR`; do
         if [ $i = template ]; then
            echo Skipping pseudo queue \"template\"
         else   
            echo updating queue: $i
            grep "^starter_method" $MSPOOLdir/$OLD_QUEUEDIR/$i 2>&1 > /dev/null
            status=$?
            if [ $status = 0 ]; then
               echo "   queue \"$i\" is already updated to $QSYST 5.1 - skipping"
               Execute cp $MSPOOLdir/$OLD_QUEUEDIR/$i $MSPOOLdir/queues/$i
            else
               Execute sed -f $CMD_DIR/queues2-51.sed $MSPOOLdir/$OLD_QUEUEDIR/$i > $MSPOOLdir/queues/$i
            fi
         fi
      done       
      echo
      $ECHO "Press <RETURN> to continue >> \c"
      read dummy
   fi

   #-----------------------------------------------------------------
   # UPGRADE: "host" complex
   #
   if [ ! -d $MSPOOLdir/complexes ]; then
      echo 
      echo ERROR: Cannot find directory: $MSPOOLdir/complexes
      echo Update failed.
      echo
      exit 1
   else
      echo "Updating \"host\" complex. Your old \"host\" complex is saved as"
      echo ""
      echo "   $MSPOOLdir/$OLD_HOSTCOMPLEX"
      echo ""

      Execute mv $MSPOOLdir/complexes/host $MSPOOLdir/$OLD_HOSTCOMPLEX
      sed -f $CMD_DIR/complexes2-51.sed $MSPOOLdir/$OLD_HOSTCOMPLEX > $MSPOOLdir/complexes/host
      grep "^swap_rsvd" $MSPOOLdir/complexes/host 2>&1 > /dev/null
      if [ $? != 0 ]; then
         echo "swap_rsvd        srsv       MEMORY 0               >=    NO          NO         0" >> $MSPOOLdir/complexes/host
         echo "swap_rate        sr         MEMORY 0               >=    NO          NO         0" >> $MSPOOLdir/complexes/host
      fi
      echo
      $ECHO "Press <RETURN> to continue >> \c"
      read dummy
   fi

   #-----------------------------------------------------------------
   # UPGRADE: exec_hosts
   #
   if [ ! -d $MSPOOLdir/exec_hosts ]; then
      echo
      echo ERROR: Cannot find directory: $MSPOOLdir/exec_hosts
      echo Update failed.
      echo
      exit 1
   else
      echo "Updating exec_hosts. Your old exec_hosts are saved in"
      echo ""
      echo "   $MSPOOLdir/$OLD_EXECDIR"
      echo ""

      Execute mv $MSPOOLdir/exec_hosts $MSPOOLdir/$OLD_EXECDIR
      Execute mkdir $MSPOOLdir/exec_hosts

      for i in `ls $MSPOOLdir/$OLD_EXECDIR`; do
         echo updating exec_host: $i
         grep "^user_lists" $MSPOOLdir/$OLD_EXECDIR/$i 2>&1 > /dev/null
         status=$?
         if [ $status = 0 ]; then
            echo "   exec_host \"$i\" is already updated to $QSYST $VERSION.2 - skipping"
            Execute cp $MSPOOLdir/$OLD_EXECDIR/$i $MSPOOLdir/exec_hosts/$i
         else
            if [ $mode = codine ]; then
               sed -f $CMD_DIR/cod_exec_hosts2-51.sed $MSPOOLdir/$OLD_EXECDIR/$i > $MSPOOLdir/exec_hosts/$i
            else
               sed -f $CMD_DIR/grd_exec_hosts2-51.sed $MSPOOLdir/$OLD_EXECDIR/$i > $MSPOOLdir/exec_hosts/$i
            fi
         fi
      done

      echo
      $ECHO "Press <RETURN> to continue >> \c"
      read dummy 
   fi

   #-----------------------------------------------------------------
   # UPGRADE: "PE" - only variable $codine_hostfile must be replaced
   #
   if [ ! -d $MSPOOLdir/pe ]; then
      echo
      echo ERROR: Cannot find directory: $MSPOOLdir/pe
      echo Update failed.
      echo
      exit 1
   else
      echo "Updating parallel environment (PE). Your old PE's are saved in"
      echo ""
      echo "   $MSPOOLdir/$OLD_PEDIR"
      echo ""

      Execute mv $MSPOOLdir/pe $MSPOOLdir/$OLD_PEDIR
      Execute mkdir $MSPOOLdir/pe

      for i in `ls $MSPOOLdir/$OLD_PEDIR`; do
         echo updating pe: $i
         sed -e 's/codine_hostfile/pe_hostfile/g' $MSPOOLdir/$OLD_PEDIR/$i > $MSPOOLdir/pe/$i
      done

      echo
      $ECHO "Press <RETURN> to continue >> \c"
      read dummy 
   fi

   #-----------------------------------------------------------------
   # UPGRADE: settings.[c]sh for GRD
   #
   if [ $mode = grd ]; then
      echo
      echo "Adding new environment variable GRD_ROOT to settings.[c]sh"
      echo "setenv GRD_ROOT $SGE_ROOT" >> $C_DIR/settings.csh
      echo "GRD_ROOT=$SGE_ROOT; export GRD_ROOT" >> $C_DIR/settings.sh

      if [ $SGE_CELL != default ]; then
         echo "Adding new environment variable GRD_CELL to settings.[c]sh"
         echo "setenv GRD_CELL $SGE_CELL" >> $C_DIR/settings.csh
         echo "GRD_CELL=$SGE_CELL; export SGE_CELL" >> $C_DIR/settings.sh
      fi

      $ECHO "For migration purposes the variables SGE_ROOT and SGE_CELL are still supported."
      $ECHO "It is recommended to only the variables GRD_ROOT and GRD_CELL in the future."
      $ECHO
      $ECHO "Press <RETURN> to continue >> \c"
      read dummy
   fi
fi
#
# end of o configuration files format changed since 5.0     

#-----------------------------------------------------------------
# NEW: startup script
#
clear
echo
echo "Creating new $QSYST startup template script"
echo "-------------$LINES------------------------"
echo

grep "^setenv COMMD_PORT" $C_DIR/settings.csh 2>&1 > /dev/null
status=$?
if [ $status = 0 ]; then
   COMMD_PORT=`grep "^setenv COMMD_PORT" $C_DIR/settings.csh | awk '{print $3}'`

   $ECHO ""
   $ECHO "Apparently your $QSYST system uses the environment variable"
   $ECHO ""
   $ECHO "      COMMD_PORT=$COMMD_PORT"
   $ECHO ""
   $ECHO "We will set this variable also in your new startup script"
   Execute $CMD_DIR/startup-51.sh $mode $COMMD_PORT \
           $SGE_ROOT/util/startup_template $C_DIR/${mode}5
else
   Execute $CMD_DIR/startup-51.sh $mode 0 \
           $SGE_ROOT/util/startup_template $C_DIR/${mode}5
fi

Execute chmod 755 $C_DIR/${mode}5

$ECHO "A new $QSYST startup script has been installed as"
$ECHO ""
$ECHO "     $C_DIR/${mode}5"
$ECHO ""
$ECHO "This script may be used on all your hosts an all architectures for"
$ECHO "starting $QSYST."
$ECHO
$ECHO "Please delete your old $QSYST startup files:"
$ECHO ""

if [ $mode = codine ]; then
   $ECHO "     /etc/codine4_startup"
   $ECHO "     /etc/init.d/codine4_startup  (if applicable)"
   $ECHO "     /sbin/init.d/codine4_startup (if applicable)"
   $ECHO "     /etc/rcX.d/S95codine4        (if applicable)"
else
   $ECHO "     /etc/grd_startup"
   $ECHO "     /etc/init.d/grd_startup   (if applicable)"
   $ECHO "     /sbin/init.d/grd_startup  (if applicable)"
   $ECHO "     /etc/rcX.d/S95grd         (if applicable)"
fi

$ECHO ""
$ECHO "on all your hosts where $QSYST is installed."
$ECHO ""
$ECHO "Press <RETURN> to continue >> \c"
read dummy 

clear

$ECHO
$ECHO "System wide installation of new $QSYST startup script"
$ECHO "----$LINES-------------------------------------------"
$ECHO
$ECHO "It is recommended to install the new startup script"
$ECHO ""
$ECHO "     $C_DIR/${mode}5"
$ECHO ""
$ECHO "on all your $QSYST hosts as"
$ECHO ""
$ECHO "     /{etc|sbin}/init.d/${mode}5"
$ECHO ""
$ECHO "and create the necessary symbolic links in"
$ECHO ""
$ECHO "     /{etc|sbin}/rcX.d/S95${mode}5"
$ECHO ""
$ECHO "Press <RETURN> to continue >> \c"
read dummy 

clear

echo
echo "             The $QSYST update procedure has completed"
echo "             ----$LINES-------------------------------"
echo "" 
echo "Please make sure to carry out all other steps as outlined in the file:"
echo ""
echo "		doc/UPGRADE-51"
echo ""

exit 0
