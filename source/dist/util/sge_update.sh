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
PATH=/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb
umask 022
  
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
   doexit=false
   for dir in jobs job_scripts zombies; do
      if [ -d $MSPOOLdir/$dir ]; then
         if [ `ls $MSPOOLdir/$dir | wc -l` -gt 0 ]; then
            Translate 0 "Found old jobs in: %s" $MSPOOLdir/$dir
            doexit=true
         fi
      fi
   done

   if [ $doexit = true ]; then
      echo
      Translate 0 "Please delete the files in the above directories"
      Translate 0 "before restarting this update procedure."
      echo
      Translate 0 "Please verify if the spool directories of your execution daemons"
      Translate 0 "do not contain any old jobs."
      echo 
      Translate 0 "You should also delete the following directories of your"
      Translate 0 "execution host spool directories:"
      echo
      Translate 0 "   %s" active_jobs/
      Translate 0 "   %s" jobs/
      Translate 0 "   %s" job_scripts/
      echo
      Translate 0 "Update failed. No changes where made."
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
   Translate 0 "Updating Grid Engine configuration to version 5.3"
   $ECHO "-------------------------------------------------"
   $ECHO
   Translate 0 "The environment variables"
   $ECHO
   Translate 0 "   SGE_ROOT"
   Translate 0 "   SGE_CELL     (only if other than \"default\")"
   Translate 0 "   COMMD_PORT   (only if service sge_commd/tcp is not used"
   $ECHO
   Translate 0 "must be set and point to the directory of your Grid Engine installation."
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
   YesNo "\n$transout" y

   if [ $? = 1 ]; then
      $ECHO
      Translate 0 "Exiting update procedure. No changes were made."
      exit 1
   fi


   C_DIR=$SGE_ROOT/$SGE_CELL/common
   MSPOOLdir=`env LC_ALL=C grep qmaster_spool_dir $C_DIR/configuration | env LC_ALL=C awk '{print $2}'`

   $CLEAR
   $ECHO
   Translate 0 "Using the following directories for this update procedure"
   $ECHO "---------------------------------------------------------"      
   $ECHO
   Translate 0 "The >common< directory in:"
   $ECHO
   Translate 0 "   %s" $C_DIR
   $ECHO
   Translate 0 "The qmaster spool directory in:"
   $ECHO
   Translate 0 "   %s" $MSPOOLdir
   Translate 4 "Do you want to use the above directories for upgrading"
   YesNo "\n$transout" y

   if [ $? = 1 ]; then
      $ECHO
      Translate 0 "Please set your environment variables and restart this script."
      Translate 0 "No changes were made."
      $ECHO
      exit 1
   fi
}

#--------------------------------------------------------------------------
# CheckPrerequisites
#    Check a bare minimum of files and directories which must exist
#
CheckPrerequisites()
{

   file1=""
   file2=""
   if [ -f $C_DIR/grd5 ]; then
      file1=$C_DIR/grd5
   fi
   if [ -f $C_DIR/codine5 ]; then
      file2=$C_DIR/codine5
   fi
   
   if [ -f $C_DIR/rcsge ]; then
      file2=$C_DIR/rcsge
   fi

   if [ "$file1" = "" -a "$file2" = "" ]; then
      $CLEAR
      echo
      Translate 0 "ERROR: startup script"
      echo
      Translate 0 "   %s or" $C_DIR/grd5
      Translate 0 "   %s" $C_DIR/codine5
      echo
      Translate 0 "does not exist."
      echo
      Translate 0 "Update failed. No changes were made."
      echo
      exit 1
   elif [ "$file1" != "" -a "$file2" != "" ]; then
      $CLEAR
      echo
      Translate 0 "ERROR: There are both startup scripts"
      echo
      Translate 0 "    %s and" $C_DIR/grd5
      Translate 0 "    %s" $C_DIR/codine5
      echo
      Translate 0 "Please delete the obsolete script and restart this update script."
      echo
      Translate 0 "Update failed. No changes were made."
      echo
      exit 1
   else
      if [ "$file1" != "" ]; then
         file="$file1"
      else
         file="$file2"
      fi
      RC_FILE_USED=$file
   fi

   env LC_ALL=C grep "COMMD_PORT=" $file 2>&1 >/dev/null

   if [ $? = 0 ]; then
      commd_port=`env LC_ALL=C grep "COMMD_PORT=" $file | tr -d '[A-Z][a-z]; _='`
      if [ $commd_port != "$COMMD_PORT" ]; then
         $CLEAR
         echo
         Translate 0 "ERROR: The >%s< variable in your startup script" "\$COMMD_PORT"
         echo
         Translate 0 "   %s" $file
         echo
         Translate 0 "is set to >%s<, while your environment variable is set to >%s<" $commd_port $COMMD_PORT
         echo
         Translate 0 "Please set your environment variable >%s< correctly and and" "\$COMMD_PORT"
         Translate 0 "restart this update script."
         echo
         Translate 0 "Update failed. No changes were made."
         echo
         exit 1
      fi
   fi

   CMD_DIR=$SGE_ROOT/util/update_commands

   if [ ! -d "$C_DIR" ]; then
      $CLEAR
      echo 
      Translate 0 "ERROR: The directory"
      echo
      Translate 0 "   %s" $C_DIR
      echo
      Translate 0 "does not exist. Please verify your environment variables and start"
      Translate 0  "this update script again."
      echo
      exit 1
   fi

   if [ ! -f $C_DIR/configuration ]; then
      $CLEAR      
      echo
      Translate 0 "ERROR: The cluster configuration file"
      echo ""
      Translate 0 "   %s" $C_DIR/configuration
      echo ""
      Translate 0  "does not exist. Please verify your environment variables and start"
      Translate 0 "this update script again."
      echo
      exit 1
   fi

   if [ ! -f $SGE_ROOT/util/startup_template ]; then
      $CLEAR
      echo
      Translate 0 "ERROR: The new Grid Engine startup template file"
      echo ""
      Translate 0 "   %s" $SGE_ROOT/util/startup_template
      echo ""
      Translate 0 "does not exist. Please verify your distribution"
      Translate 0 "and start this update script again."
      echo
      exit 1
   fi

   if [ ! -d "$MSPOOLdir" ]; then
      $CLEAR
      echo
      Translate 0 "ERROR: Apparently your qmaster spool directory"
      echo ""
      Translate 0 "   %s" $MSPOOLdir
      echo ""
      Translate 0 "which is defined in the file"
      echo ""
      echo "   %s" $C_DIR/configuration
      echo ""
      Translate 0 "does not exist. Please verify your configuration and start"
      Translate 0 "this update script again."
      echo
      exit 1
   fi
}

#-------------------------------------------------------------------------
# GetGidRange
#    
GetGidRange()
{
   done=false
   while [ $done = false ]; do
      $CLEAR
      $ECHO ""
      Translate 0 "%s group id range" SGE
      $ECHO "------------------"
      $ECHO ""
      Translate 0 "When jobs are started under the control of %s an additional group id is set" SGE
      Translate 0 "on platforms which do not support jobs. This is done to provide maximum control"
      Translate 0 "for %s jobs." SGE
      Translate 0 "This additional UNIX group id range MUST be unused group id's in your system."
      Translate 0 "Each job will be assigned a unique id during the time it is running."
      Translate 0 "Therefore you need to provide a range of id's which will be assigned"
      Translate 0 "dynamically for jobs."
      Translate 0 "The range must be big enough to provide enough numbers for the maximum number"
      Translate 0 "of %s jobs running at a single moment on a single host. E.g. a range like" SGE
      $ECHO ""
      Translate 0 "   20000-20050"
      $ECHO ""
      Translate 0 "means, that %s will use the group ids from 20000-20050 and provides a range" SGE
      Translate 0 "for 50 %s jobs at the same time on a single host." SGE
      Translate 0 "You can change at any time the group id range in your cluster configuration."
      $ECHO ""
      Translate 2 "Please enter a range: "

      GID_RANGE=`Enter ""`

      if [ "$GID_RANGE" != "" ]; then
         Translate 4 "Do you want to use >%s< as gid range." "$GID_RANGE"
         YesNo "\n$transout" y
         if [ $? = 0 ]; then
            done=true
         fi
      fi
   done
}

#-------------------------------------------------------------------------
# GetCompatibilityMode
#    
GetCompatibilityMode()
{
   done=false
   while [ $done = false ]; do
      $CLEAR
      $ECHO ""
      Translate 0 "Compatibility mode for environment variables"
      $ECHO       "--------------------------------------------"
      $ECHO ""
      Translate 0 "Grid Engine version prior release 5.3 used environment variables" 
      Translate 0 "with the prefix >CODINE_<, >COD_<, >GRD_<, e.g."
      $ECHO ""
      Translate 0 "   \$CODINE_ROOT, \$GRD_ROOT, \$COD_O_HOME etc."
      $ECHO ""
      Translate 0 "Grid Engine 5.3 uses environment variables beginning with >SGE_<."
      $ECHO ""
      Translate 0 "This version (but not version 6.0) supports a compatibility mode where for job"
      Translate 0 "execution the old variables are set in addition to the new variables. It is not"
      Translate 0 "recommended to use this compatibility mode. However if migration for your users"
      Translate 0 "to the new variables is difficult you can enable the support."
      $ECHO ""
      Translate 0 "Please enter now what compatibility mode you want to enable."
      $ECHO ""
      Translate 0 "   0  - no compatibility mode  (default, use >SGE_< prefix only)"
      Translate 0 "   1  - support COD_/CODINE_ variable prefix"
      Translate 0 "   2  - support GRD_ variable prefix"
      Translate 0 "   3  - support COD_/CODINE_/GRD variable prefix"
      $ECHO ""
      Translate 2 "Please enter [0-3] >> "
      select=`Enter 0`

      $CLEAR
      echo
      valid=true
      if [ $select = 0 ]; then
         Translate 0 "We will not enable the variable compatibility mode"
         SGE_COMPATIBILITY=none
      elif [ $select = 1 ]; then
         Translate 0 "We will enable the variable compatibility mode for the COD_/CODINE_ prefix"
         SGE_COMPATIBILITY="SET_COD_ENV=true"
      elif [ $select = 2 ]; then
         Translate 0 "We will enable the variable compatibility mode for the GRD_ prefix"
         SGE_COMPATIBILITY="SET_GRD_ENV=true"
      elif [ $select = 3 ]; then
         Translate 0 "We will enable the variable compatibility mode for the COD_/CODINE_/GRD prefix"
         SGE_COMPATIBILITY="SET_COD_ENV=true SET_GRD_ENV=true"
      else
         Translate 0 "Invalid input."
         valid=false   
      fi

      if [ $valid = true ]; then
         Translate 4 "Do you want to use the selected compatibility mode" y
         YesNo "\n$transout" y
         if [ $? = 0 ]; then
            done=true
            WaitClear clear
         fi
      else
         WaitClear clear
      fi
   done
}


#--------------------------------------------------------------------------
# CheckUpgradeType
#    find out if user want to upgrade to SGE or SGEEE
CheckUpgradeType()
{
  $CLEAR
 
  chk_sgeee=false
  chk_sge=false

   egrep "grd|sgeee" $C_DIR/product_mode 2>&1 >/dev/null
   if [ $? = 0 ]; then
      chk_sgeee=true
   else
      egrep "codine|sge" $C_DIR/product_mode 2>&1 >/dev/null
      if [ $? = 0 ]; then
         chk_sge=true
      fi
   fi

   if [ $chk_sgeee = false -a $chk_sge = false ]; then
      echo
      Translate 0 "Can't read your product mode file:"
      echo
      Translate 0 "   %s" $C_DIR/product_mode 
      echo
      Translate 0 "Can't read string >codine< or >grd< >sge< >sgeee< in >%s< file." product_mode
      echo
      Translate 0 "Update failed. Exit."
      exit 1
    fi

   if [ $chk_sge = true ]; then
      OLD_SGE_MODE=sge
      echo
      Translate 0 "You can upgrade to Grid Engine Enterprise Edition (%s)" SGEEE
      echo        "---------------------------------------------------------"
      echo
      Translate 0 "Your old Grid Engine installation is a %s or %s system." CODINE SGE
      echo
      Translate 0 "It is possible to upgrade your installation to a Grid Engine"
      Translate 1 "Enterprise Edition (%s) system. You should only upgrade your installation" SGEEE
      Translate 0 "to a >%s< system if you are familiar with the concepts of %s" SGEEE SGEEE
      echo
      Translate 0 "It will be neccessary to carry out a couple of further configuration"
      Translate 0 "steps if you want to make use of the additional %s features." SGEEE
      Translate 4 "Do you want to upgrade to %s" SGEEE
      YesNo "\n$transout" n
      if [ $? = 0 ]; then
         SGE_MODE=sgeee
         echo
         Translate 0 "We will upgrade your installation to %s." SGEEE
      else
         SGE_MODE=sge
         echo
         Translate 0 "Your installation will be upgraded to SGE."
      fi
      env LC_ALL=C grep '^gid_range' $C_DIR/configuration $2 2>&1 >/dev/null
      if [ $? != 0 ]; then
         GetGidRange
      else
         GID_RANGE=notused
      fi
   else
      OLD_SGE_MODE=sgeee
      SGE_MODE=sgeee
      GID_RANGE=notused
      echo
      Translate 0 "Your old Grid Engine installation is a GRD or SGEEE system."
      echo
      Translate 0 "We will update your system to >%s<" "SGEEE 5.3beta2"
   fi
   echo
   WaitClear clear

   GetCompatibilityMode
}


#--------------------------------------------------------------------------
# MakeBackupDirs
#  create backup directory hierarchy in "common" dir
#  exit if there'sa failure
#
MakeBackupDirs()
{
   backdirbase=`env LC_ALL=C date "+%Y%m%d-%H:%M:%S"`

   if [ "$backdirbase" = "" ]; then
      echo
      Translate 0 "ERROR: cannot define a backup directory name with the command"
      echo
      Translate 0 "   >%s<" "env LC_ALL=C date \"+%Y%m%d-%H:%M:%S\""
      echo
      Translate 0 "Update failed. No changes where made."
      echo
      exit 1
   fi

   BACKUP_DIR_COMMON=$C_DIR/$backdirbase/common
   BACKUP_DIR_COMPLEXES=$C_DIR/$backdirbase/complexes
   BACKUP_DIR_EXEC_HOSTS=$C_DIR/$backdirbase/exec_hosts
   BACKUP_DIR_QUEUES=$C_DIR/$backdirbase/queues
   BACKUP_DIR_USERSETS=$C_DIR/$backdirbase/usersets

   Execute $MKDIR $C_DIR/$backdirbase
   Execute $MKDIR $BACKUP_DIR_COMMON
   Execute $MKDIR $BACKUP_DIR_COMPLEXES
   Execute $MKDIR $BACKUP_DIR_EXEC_HOSTS
   Execute $MKDIR $BACKUP_DIR_QUEUES
   Execute $MKDIR $BACKUP_DIR_USERSETS
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
CheckPrerequisites
CheckUpgradeType

Translate 4 "Do you want to start the update procedure"
YesNo "$transout" y
if [ $? != 0 ]; then
   echo
   Translate 0 "Exiting update procedure. No changes were made."
   exit 1
fi

CheckForOldJobs
MakeBackupDirs

echo
Translate 0 "Deleting obsolete files..."

#-----------------------------------------------------------------
# DELETE: cleanup of 5.0, 5.1, 5.2 files
#

rm -f $SGE_ROOT/VERSION_5.0* 2>&1 >/dev/null
rm -f $SGE_ROOT/VERSION_5.1* 2>&1 >/dev/null
rm -f $SGE_ROOT/VERSION_5.2* 2>&1 >/dev/null

if [ -d $SGE_ROOT/util/commands50 ]; then
   rm -rf $SGE_ROOT/util/commands50
fi

if [ -d $SGE_ROOT/util/commands51 ]; then
   rm -rf $SGE_ROOT/util/commands51
fi

if [ -f  $SGE_ROOT/util/cod_update-50.sh ]; then
   rm -f $SGE_ROOT/util/cod_update-50.sh
fi

if [ -f  $SGE_ROOT/util/grd_update-50.sh ]; then
   rm -f $SGE_ROOT/util/grd_update-50.sh
fi

#-----------------------------------------------------------------
# DELETE: history directory tree
#
if [ -d $C_DIR/history ]; then
   Translate 0 "Deleting history directory tree: %s" $C_DIR/history
   Execute rm -rf $C_DIR/history
   Execute mkdir $C_DIR/history
fi

#-----------------------------------------------------------------
# DELETE: qsi directory tree
#
if [ -d $C_DIR/qsi ]; then
   Translate 0 "Deleting directory tree: %s" $C_DIR/qsi
   Execute rm -rf $C_DIR/qsi
fi

#-----------------------------------------------------------------
# DELETE: statistics file
#
if [ -f $C_DIR/statistics ]; then
   Translate 0 "Deleting file: %s" $C_DIR/statistics
   Execute rm $C_DIR/statistics
fi        

#-----------------------------------------------------------------
# DELETE: sched_runlog file
#
if [ -f $C_DIR/sched_runlog ]; then
   Translate 0 "Deleting file: %s" $C_DIR/sched_runlog
   Execute rm $C_DIR/sched_runlog
fi        

#-----------------------------------------------------------------
# DELETE: license file
#
if [ -f $C_DIR/license ]; then
   Translate 0 "Deleting file: %s" $C_DIR/license
   Execute rm $C_DIR/license
fi        

#-----------------------------------------------------------------
# RENAME: codine_aliases file
#
if [ -f $C_DIR/codine_aliases ]; then
   Translate 0 "Renaming file: %s" $C_DIR/codine_aliases
   Execute $MV $C_DIR/codine_aliases $C_DIR/sge_aliases
fi

#-----------------------------------------------------------------
# RENAME: cod_request file
#
if [ -f $C_DIR/cod_request ]; then
   Translate 0 "Renaming file: %s" $C_DIR/cod_request
   Execute $MV $C_DIR/cod_request $C_DIR/sge_request
fi

#-----------------------------------------------------------------
# UPGRADE: global cluster configuration
#
Translate 0 "Updating global cluster configuration file"

Execute $CP $C_DIR/configuration $BACKUP_DIR_COMMON
Execute $RM $C_DIR/configuration
Execute $TOUCH $C_DIR/configuration
$CMD_DIR/configuration.sh $SGE_MODE $BACKUP_DIR_COMMON/configuration $C_DIR/configuration \
                          $GID_RANGE "$SGE_COMPATIBILITY"
if [ $? != 0 ]; then
   Translate 0 "Failed updating cluster config: %s" $C_DIR/configuration
fi

#-----------------------------------------------------------------
# CREATE: settings.[c]sh
#
Translate 0 "Create new settings.[c]sh files"
Execute $CP $C_DIR/settings.sh $BACKUP_DIR_COMMON
Execute $CP $C_DIR/settings.csh $BACKUP_DIR_COMMON
$SGE_ROOT/util/create_settings.sh $C_DIR

#-----------------------------------------------------------------
# CREATE: product_mode file
#    be careful about the "-" combinations in this file, do not
#    destroy any additional settings in this file
#
Translate 0 "Creating new >%s< file" product_mode
Execute $CP $C_DIR/product_mode $BACKUP_DIR_COMMON
Execute $RM $C_DIR/product_mode

if [ $OLD_SGE_MODE = sgeee ]; then
   grep sgeee $BACKUP_DIR_COMMON/product_mode 2>&1 > /dev/null
   if [ $? != 0 ]; then
      grep grd $BACKUP_DIR_COMMON/product_mode 2>&1 > /dev/null
      if [ $? = 0 ]; then
         sed -e 's/grd/sgeee/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
      else
         sed -e 's/sge/sgeee/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
      fi
   else
      Execute $CP $BACKUP_DIR_COMMON/product_mode $C_DIR/product_mode
   fi
elif [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
   grep codine $BACKUP_DIR_COMMON/product_mode 2>&1 > /dev/null
   if [ $? = 0 ]; then
      sed -e 's/codine/sgeee/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
   else
      sed -e -e 's/sge/sgeee/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
   fi
elif [ $OLD_SGE_MODE = sge -a $SGE_MODE = sge ]; then
   sed -e 's/codine/sge/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
else
   Translate "Ooops, this should not happen - please check your product_mode file"
   Execute $CP $BACKUP_DIR_COMMON/product_mode $C_DIR/product_mode
fi

#-----------------------------------------------------------------
# UPGRADE: accounting file
#
if [ -f $C_DIR/accounting ]; then
   Translate 0 "Updating accounting file"
   Execute $CP $C_DIR/accounting $BACKUP_DIR_COMMON
   Execute $RM $C_DIR/accounting
   env LC_ALL=C $AWK -f $CMD_DIR/accounting.awk $BACKUP_DIR_COMMON/accounting > $C_DIR/accounting
   if [ $? != 0 ]; then
      Translate 0 "Failed updating accounting file: %s" $C_DIR/accounting
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: scheduler configuration
#
if [ -f $C_DIR/sched_configuration ]; then
   Translate 0 "Updating scheduler configuration"
   Execute $CP $C_DIR/sched_configuration $BACKUP_DIR_COMMON/sched_configuration
   Execute $RM $C_DIR/sched_configuration
   sed -e '/^maxgjobs/d' -e 's/grd_schedule_interval/sgeee_schedule_interval/' \
          $BACKUP_DIR_COMMON/sched_configuration > $C_DIR/sched_configuration
   if [ $SGE_MODE = sgeee -a $OLD_SGE_MODE = sge ]; then
      cat $CMD_DIR/sgeee-sched_configuration.template >> $C_DIR/sched_configuration  
   fi
fi

#-----------------------------------------------------------------
# Create startup script "rcsge"
#
Translate 0 "Creating new startup script: %s" $C_DIR/rcsge
Execute $CP $RC_FILE_USED $BACKUP_DIR_COMMON
Execute $RM $RC_FILE_USED

TMP_V5_STARTUP_FILE=/tmp/rcsge.$$
if [ -f ${TMP_V5_STARTUP_FILE}.0 ]; then
   Execute rm ${TMP_V5_STARTUP_FILE}.0
fi
Execute sed -e "s%=GENROOT%=$SGE_ROOT%g" \
            -e "s%=GENCELL%=$SGE_CELL%g" \
            -e "/#+-#+-#+-#-/,/#-#-#-#-#-#/d" $SGE_ROOT/util/startup_template > ${TMP_V5_STARTUP_FILE}.0
if [ "$COMMD_PORT" != "" ]; then
   Execute sed -e "s/=GENCOMMD_PORT/=$COMMD_PORT/" ${TMP_V5_STARTUP_FILE}.0 > $C_DIR/rcsge
else
   Execute sed -e "/=GENCOMMD_PORT/d" ${TMP_V5_STARTUP_FILE}.0 > $C_DIR/rcsge
fi

$RM ${TMP_V5_STARTUP_FILE}.0

Execute chmod 755 $C_DIR/rcsge

#-----------------------------------------------------------------
# UPGRADE: queues/
#
if [ ! -d $MSPOOLdir/queues ]; then
   Translate 0 "No queue directory found: %s" $MSPOOLdir/queues
else
   Translate 0 "Updating queues"

   if [ "`ls $MSPOOLdir/queues`" != "" ]; then
      Execute $CP $MSPOOLdir/queues/* $BACKUP_DIR_QUEUES
      Execute $RM -f $MSPOOLdir/queues/.??*
      Execute $RM -f $MSPOOLdir/queues/*
      
      for i in `ls $BACKUP_DIR_QUEUES`; do
         if [ $i = template ]; then
            Translate 0 "   Skipping pseudo queue \"template\""
         else   
            Translate 0 "   updating queue: %s" $i
            env LC_ALL=C grep "^max_migr_time" $BACKUP_DIR_QUEUES/$i 2>&1 > /dev/null
            if [ $? != 0 ]; then
               Translate 0 "      queue is already updated - skipping"
               Execute cp $BACKUP_DIR_QUEUES/$i $MSPOOLdir/queues
            else
               if [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
                  Execute sed -f $CMD_DIR/sge2sgeee-queues.sed $BACKUP_DIR_QUEUES/$i > $MSPOOLdir/queues/$i
               else
                  Execute sed -f $CMD_DIR/queues.sed $BACKUP_DIR_QUEUES/$i > $MSPOOLdir/queues/$i
               fi
            fi
         fi
      done
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: exec_hosts/
#
if [ ! -d $MSPOOLdir/exec_hosts ]; then
   Translate 0 "No queue directory found: %s" $MSPOOLdir/exec_hosts
else
   Translate 0 "Updating exec_hosts"

   if [ "`ls $MSPOOLdir/exec_hosts`" != "" ]; then
      Execute cp $MSPOOLdir/exec_hosts/* $BACKUP_DIR_EXEC_HOSTS
      Execute $RM -f $MSPOOLdir/exec_hosts/.??*
      Execute $RM -f $MSPOOLdir/exec_hosts/*
      
      for i in `ls $BACKUP_DIR_EXEC_HOSTS`; do
         Translate 0 "   updating exec host: %s" $i
         env LC_ALL=C grep "^real_host_name" $BACKUP_DIR_EXEC_HOSTS/$i 2>&1 > /dev/null
         if [ $? != 0 ]; then
            Translate 0 "      exec host is already updated - skipping"
            Execute cp $BACKUP_DIR_EXEC_HOSTS/$i $MSPOOLdir/exec_hosts
         else
            if [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
               Execute sed -e '/^reschedule_unknown_list/d' -f $CMD_DIR/sge2sgeee-exec_hosts.sed $BACKUP_DIR_EXEC_HOSTS/$i > $MSPOOLdir/exec_hosts/$i
            else
               Execute sed -e '/^reschedule_unknown_list/d' -f $CMD_DIR/exec_hosts.sed $BACKUP_DIR_EXEC_HOSTS/$i > $MSPOOLdir/exec_hosts/$i
            fi
         fi
      done
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: "queue" complex
#
if [ ! -d $MSPOOLdir/complexes ]; then
   Translate 0 "No complex directory found: %s" $MSPOOLdir/complexes
else
   Translate 0 "Updating >%s< complex" queue
   Execute cp $MSPOOLdir/complexes/queue $BACKUP_DIR_COMPLEXES
   sed -e "/^priority */d" $BACKUP_DIR_COMPLEXES/queue > $MSPOOLdir/complexes/queue
fi

#-----------------------------------------------------------------
# UPGRADE: usersets
#
if [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
   Translate 0 "Updating usersets"
   if [ "`ls $MSPOOLdir/usersets" != "" ]; then

      Execute $CP $MSPOOLdir/usersets/* $BACKUP_DIR_USERSETS
      Execute $RM -f $MSPOOLdir/usersets/*
      Execute $RM -f $MSPOOLdir/usersets/.??*

      for i in `ls $BACKUP_DIR_USERSETS`; do
         Translate 0 "   updating userset: %s" $i
         env LC_ALL=C grep "^type" $BACKUP_DIR_USERSETS/$i 2>&1 > /dev/null
         if [ $? = 0 ]; then
            Translate 0 "      userset is already updated - skipping"
            Execute cp $BACKUP_DIR_USERSETS/$i $MSPOOLdir/usersets
         else
            Execute sed -f $CMD_DIR/usersets.sed $BACKUP_DIR_USERSETS/$i > $MSPOOLdir/usersets/$i
         fi
       done   
   fi

   if [ ! -f $MSPOOLdir/usersets/defaultdepartment ]; then
      Translate 0 "adding %s >%s< userset" SGEEE defaultdepartment
      Execute $CP $SGE_ROOT/util/resources/usersets/defaultdepartment $MSPOOLdir/usersets
   fi
   if [ ! -f $MSPOOLdir/usersets/deadlineusers ]; then
      Translate 0 "adding %s >%s< userset" SGEEE deadlineusers
      Execute $CP $SGE_ROOT/util/resources/usersets/deadlineusers $MSPOOLdir/usersets
   fi
fi

echo
WaitClear clear

echo
Translate 0 "A new Grid Engine startup script has been installed as"
$ECHO ""
Translate 0 "   %s" $C_DIR/rcsge
$ECHO ""
Translate 0 "This script may be used on all your hosts an all architectures for"
Translate 0 "starting Grid Engine."
$ECHO
Translate 0 "Please delete your old $QSYST startup files (not all of"
Translate 0 "of these files will exist on your systems):"
$ECHO ""
Translate 0 "     /etc/codine5_startup"        
Translate 0 "     /etc/init.d/codine5_startup"
Translate 0 "     /sbin/init.d/codine5_startup"
Translate 0 "     /etc/rcX.d/S95codine5"
Translate 0 "     /etc/grd_startup"
Translate 0 "     /etc/init.d/grd_startup"
Translate 0 "     /sbin/init.d/grd_startup"
Translate 0 "     /etc/rcX.d/S95grd"
$ECHO ""
Translate 0 "on all your hosts where Grid Engine is installed."
echo
WaitClear clear


Translate 0 "System wide installation of new Grid Engine startup script"
$ECHO "----------------------------------------------------------"
$ECHO
Translate 0 "It is recommended to install the new startup script"
$ECHO ""
Translate 0 "     $C_DIR/rcsge"
$ECHO ""
Translate 0 "on all your $QSYST hosts as"
$ECHO ""
Translate 0 "     /{etc|sbin}/init.d/rcsge"
$ECHO ""
Translate 0 "and create the necessary symbolic links in"
$ECHO ""
Translate 0 "     /{etc|sbin}/rcX.d/S95rcsge"
$ECHO ""
WaitClear clear

Translate 0 "The Grid Engine update procedure has completed"
$ECHO "----------------------------------------------"
$ECHO "" 
Translate 0 "Please make sure to carry out all other steps as outlined in the file:"
$ECHO ""
Translate 0 "   %s" doc/UPGRADE
$ECHO ""
Translate 0 "of the Grid Engine distribution."

exit 0
