#!/bin/sh
#
# Update CODINE/GRD/SGE(EE) file to version 5.3
#
# (c) 2002 Sun Microsystems, Inc. Use is subject to license terms.  
#
# set -x   

# Reset PATH to a safe value
#
PATH=/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bsd
umask 022
  
# Easy way to prevent clearing of screen
#
CLEAR=clear
#CLEAR=:


# must be set to false in this script
#
autoinst=false

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
            $INFOTEXT "Found old jobs in: %s" $MSPOOLdir/$dir
            doexit=true
         fi
      fi
   done

   if [ $doexit = true ]; then
      echo
      $INFOTEXT "Please delete the files in the above directories"
      $INFOTEXT "before restarting this update procedure."
      echo
      $INFOTEXT "Please verify if the spool directories of your execution daemons"
      $INFOTEXT "do not contain any old jobs."
      echo 
      $INFOTEXT "You should also delete the following directories of your"
      $INFOTEXT "execution host spool directories:"
      echo
      $INFOTEXT "   %s" active_jobs/
      $INFOTEXT "   %s" jobs/
      $INFOTEXT "   %s" job_scripts/
      echo
      $INFOTEXT "Update failed. No changes where made."
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
      $INFOTEXT "This is an adminuser installation"
      $ECHO "---------------------------------"
      $ECHO
      $INFOTEXT "According to your global cluster configuration your Grid Engine"
      $INFOTEXT "installation used the adminuser name"
      $ECHO
      $INFOTEXT "   >%s<" $ADMINUSER
      $ECHO
      $INFOTEXT "to create its spool files. Please login as user >%s<" $ADMINUSER
      $INFOTEXT "and restart this update script."
      $ECHO
      $INFOTEXT "Exiting update procedure. No changes were made."
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
   $INFOTEXT "Updating Grid Engine configuration to version 5.3"
   $ECHO "-------------------------------------------------"
   $ECHO
   $INFOTEXT "The environment variables"
   $ECHO
   $INFOTEXT "   SGE_ROOT"
   $INFOTEXT "   SGE_CELL     (only if other than \"default\")"
   $INFOTEXT "   COMMD_PORT   (only if service sge_commd/tcp is not used"
   $ECHO
   $INFOTEXT "must be set and point to the directory of your Grid Engine installation."
   $INFOTEXT "You may not use this script unless you are upgrading from:"
   $ECHO
   $INFOTEXT "   CODINE versions 5.0.x and 5.1.x"
   $INFOTEXT "   GRD versions 5.0.x and 5.1.x"
   $INFOTEXT "   SGE 5.2.x"
   $INFOTEXT "   SGE(EE) 5.3betax"
   $ECHO
   $INFOTEXT "Please read the upgrade documentation"
   $ECHO
   $INFOTEXT "   %s" \$SGE_ROOT/doc/UPGRADE
   $ECHO
   $INFOTEXT "before starting this update procedure."


   $INFOTEXT -auto $autoinst -ask "y" "n" -def "n" -n \
             "Do you want to proceed with the update proecdure (y/n) [n] >> "

   if [ $? = 1 ]; then
      $ECHO
      $INFOTEXT "Exiting update procedure. No changes were made."
      exit 1
   fi


   C_DIR=$SGE_ROOT/$SGE_CELL/common
   MSPOOLdir=`env LC_ALL=C grep qmaster_spool_dir $C_DIR/configuration | env LC_ALL=C awk '{print $2}'`

   $CLEAR
   $ECHO
   $INFOTEXT "Using the following directories for this update procedure"
   $ECHO "---------------------------------------------------------"      
   $ECHO
   $INFOTEXT "The >common< directory in:"
   $ECHO
   $INFOTEXT "   %s" $C_DIR
   $ECHO
   $INFOTEXT "The qmaster spool directory in:"
   $ECHO
   $INFOTEXT "   %s" $MSPOOLdir

   $INFOTEXT -auto $autoinst -ask "y" "n" -def "y" -n \
             "Do you want to use the above directories for upgrading (y/n) [y] >> "

   if [ $? = 1 ]; then
      $ECHO
      $INFOTEXT "Please set your environment variables and restart this script."
      $INFOTEXT "No changes were made."
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
      $INFOTEXT "ERROR: startup script"
      echo
      $INFOTEXT "   %s or" $C_DIR/grd5
      $INFOTEXT "   %s" $C_DIR/codine5
      echo
      $INFOTEXT "does not exist."
      echo
      $INFOTEXT "Update failed. No changes were made."
      echo
      exit 1
   elif [ "$file1" != "" -a "$file2" != "" ]; then
      $CLEAR
      echo
      $INFOTEXT "ERROR: There are both startup scripts"
      echo
      $INFOTEXT "    %s and" $C_DIR/grd5
      $INFOTEXT "    %s" $C_DIR/codine5
      echo
      $INFOTEXT "Please delete the obsolete script and restart this update script."
      echo
      $INFOTEXT "Update failed. No changes were made."
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
         $INFOTEXT "ERROR: The >%s< variable in your startup script" "\$COMMD_PORT"
         echo
         $INFOTEXT "   %s" $file
         echo
         $INFOTEXT "is set to >%s<, while your environment variable is set to >%s<" $commd_port $COMMD_PORT
         echo
         $INFOTEXT "Please set your environment variable >%s< correctly and and" "\$COMMD_PORT"
         $INFOTEXT "restart this update script."
         echo
         $INFOTEXT "Update failed. No changes were made."
         echo
         exit 1
      fi
   fi

   CMD_DIR=$SGE_ROOT/util/update_commands

   if [ ! -d "$C_DIR" ]; then
      $CLEAR
      echo 
      $INFOTEXT "ERROR: The directory"
      echo
      $INFOTEXT "   %s" $C_DIR
      echo
      $INFOTEXT "does not exist. Please verify your environment variables and start"
      $INFOTEXT  "this update script again."
      echo
      exit 1
   fi

   if [ ! -f $C_DIR/configuration ]; then
      $CLEAR      
      echo
      $INFOTEXT "ERROR: The cluster configuration file"
      echo ""
      $INFOTEXT "   %s" $C_DIR/configuration
      echo ""
      $INFOTEXT  "does not exist. Please verify your environment variables and start"
      $INFOTEXT "this update script again."
      echo
      exit 1
   fi

   if [ ! -f $SGE_ROOT/util/startup_template ]; then
      $CLEAR
      echo
      $INFOTEXT "ERROR: The new Grid Engine startup template file"
      echo ""
      $INFOTEXT "   %s" $SGE_ROOT/util/startup_template
      echo ""
      $INFOTEXT "does not exist. Please verify your distribution"
      $INFOTEXT "and start this update script again."
      echo
      exit 1
   fi

   if [ ! -d "$MSPOOLdir" ]; then
      $CLEAR
      echo
      $INFOTEXT "ERROR: Apparently your qmaster spool directory"
      echo ""
      $INFOTEXT "   %s" $MSPOOLdir
      echo ""
      $INFOTEXT "which is defined in the file"
      echo ""
      echo "   %s" $C_DIR/configuration
      echo ""
      $INFOTEXT "does not exist. Please verify your configuration and start"
      $INFOTEXT "this update script again."
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
      $INFOTEXT "%s group id range" SGE
      $ECHO "------------------"
      $ECHO ""
      $INFOTEXT "When jobs are started under the control of %s an additional group id is set" SGE
      $INFOTEXT "on platforms which do not support jobs. This is done to provide maximum control"
      $INFOTEXT "for %s jobs." SGE
      $INFOTEXT "This additional UNIX group id range MUST be unused group id's in your system."
      $INFOTEXT "Each job will be assigned a unique id during the time it is running."
      $INFOTEXT "Therefore you need to provide a range of id's which will be assigned"
      $INFOTEXT "dynamically for jobs."
      $INFOTEXT "The range must be big enough to provide enough numbers for the maximum number"
      $INFOTEXT "of %s jobs running at a single moment on a single host. E.g. a range like" SGE
      $ECHO ""
      $INFOTEXT "   20000-20050"
      $ECHO ""
      $INFOTEXT "means, that %s will use the group ids from 20000-20050 and provides a range" SGE
      $INFOTEXT "for 50 %s jobs at the same time on a single host." SGE
      $INFOTEXT "You can change at any time the group id range in your cluster configuration."
      $ECHO ""
      $INFOTEXT -n "Please enter a range >> "

      GID_RANGE=`Enter ""`

      if [ "$GID_RANGE" != "" ]; then
         $INFOTEXT -auto $autoinst -ask "y" "n" -def "y" -n \
                   "Do you want to use >%s< as gid range (y/n) [y] >> " "$GID_RANGE"
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
      $INFOTEXT "Compatibility mode for environment variables"
      $ECHO       "--------------------------------------------"
      $ECHO ""
      $INFOTEXT "Grid Engine version prior release 5.3 used environment variables" 
      $INFOTEXT "with the prefix >CODINE_<, >COD_<, >GRD_<, e.g."
      $ECHO ""
      $INFOTEXT "   \$CODINE_ROOT, \$GRD_ROOT, \$COD_O_HOME etc."
      $ECHO ""
      $INFOTEXT "Grid Engine 5.3 uses environment variables beginning with >SGE_<."
      $ECHO ""
      $INFOTEXT "This version (but not version 6.0) supports a compatibility mode where for job"
      $INFOTEXT "execution the old variables are set in addition to the new variables. It is not"
      $INFOTEXT "recommended to use this compatibility mode. However if migration for your users"
      $INFOTEXT "to the new variables is difficult you can enable the support."
      $ECHO ""
      $INFOTEXT "Please enter now what compatibility mode you want to enable."
      $ECHO ""
      $INFOTEXT "   0  - no compatibility mode  (default, use >SGE_< prefix only)"
      $INFOTEXT "   1  - support COD_/CODINE_ variable prefix"
      $INFOTEXT "   2  - support GRD_ variable prefix"
      $INFOTEXT "   3  - support COD_/CODINE_/GRD variable prefix"
      $ECHO ""
      $INFOTEXT -n "Please enter [0-3] >> "
      select=`Enter 0`

      $CLEAR
      echo
      valid=true
      if [ $select = 0 ]; then
         $INFOTEXT "We will not enable the variable compatibility mode"
         SGE_COMPATIBILITY=none
      elif [ $select = 1 ]; then
         $INFOTEXT "We will enable the variable compatibility mode for the COD_/CODINE_ prefix"
         SGE_COMPATIBILITY="SET_COD_ENV=true"
      elif [ $select = 2 ]; then
         $INFOTEXT "We will enable the variable compatibility mode for the GRD_ prefix"
         SGE_COMPATIBILITY="SET_GRD_ENV=true"
      elif [ $select = 3 ]; then
         $INFOTEXT "We will enable the variable compatibility mode for the COD_/CODINE_/GRD prefix"
         SGE_COMPATIBILITY="SET_COD_ENV=true SET_GRD_ENV=true"
      else
         $INFOTEXT "Invalid input."
         valid=false   
      fi

      if [ $valid = true ]; then
         $INFOTEXT -auto $autoinst -ask "y" "n" -def "y" -n \
                   "Do you want to use the selected compatibility mode (y/n) [y] >> "
         if [ $? = 0 ]; then
            done=true
            $INFOTEXT -wait -auto $autoinst -n "\nHit <RETURN> to continue >> "
            $CLEAR
         fi
      else
         $INFOTEXT -wait -auto $autoinst -n "\nHit <RETURN> to continue >> "
         $CLEAR
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
      $INFOTEXT "Can't read your product mode file:"
      echo
      $INFOTEXT "   %s" $C_DIR/product_mode 
      echo
      $INFOTEXT "Can't read string >codine< or >grd< >sge< >sgeee< in >%s< file." product_mode
      echo
      $INFOTEXT "Update failed. Exit."
      exit 1
    fi

   if [ $chk_sge = true ]; then
      OLD_SGE_MODE=sge
      echo
      $INFOTEXT "You can upgrade to Grid Engine Enterprise Edition (%s)" SGEEE
      echo        "---------------------------------------------------------"
      echo
      $INFOTEXT "Your old Grid Engine installation is a %s or %s system." CODINE SGE
      echo
      $INFOTEXT "It is possible to upgrade your installation to a Grid Engine"
      $INFOTEXT "Enterprise Edition (%s) system. You should only upgrade your installation" SGEEE
      $INFOTEXT "to a >%s< system if you are familiar with the concepts of %s" SGEEE SGEEE
      echo
      $INFOTEXT "It will be neccessary to carry out a couple of further configuration"
      $INFOTEXT "steps if you want to make use of the additional SGEEE features."

      $INFOTEXT -auto $autoinst -ask "y" "n" -def "n" -n \
                "Do you want to upgrade to SGEEE (y/n) [n] >> "
      if [ $? = 0 ]; then
         SGE_MODE=sgeee
         echo
         $INFOTEXT "We will upgrade your installation to %s." SGEEE
      else
         SGE_MODE=sge
         echo
         $INFOTEXT "Your installation will be upgraded to SGE."
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
      $INFOTEXT "Your old Grid Engine installation is a GRD or SGEEE system."
      echo
      $INFOTEXT "We will update your system to >%s<" "SGEEE 5.3"
   fi

   $INFOTEXT -wait -auto $autoinst -n "\nHit <RETURN> to continue >> "  
   $CLEAR

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
      $INFOTEXT "ERROR: cannot define a backup directory name with the command"
      echo
      $INFOTEXT "   >%s<" "env LC_ALL=C date \"+%Y%m%d-%H:%M:%S\""
      echo
      $INFOTEXT "Update failed. No changes where made."
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

#---------------------------------------
# setup INFOTEXT begin
#---------------------------------------

V5BIN=$SGE_ROOT/bin/$ARCH
V5UTILBIN=$SGE_ROOT/utilbin/$ARCH
# INFOTXT_DUMMY is needed by message parsing script
# which is looking for $INFOTEXT and would report
# errors in the next command. Please use INFOTXT_DUMMY
# instead of using $INFOTEXT

INFOTXT_DUMMY=$V5UTILBIN/infotext
INFOTEXT=$INFOTXT_DUMMY
if [ ! -x $INFOTXT_DUMMY ]; then
   echo "can't find binary \"$INFOTXT_DUMMY\""
   echo "Installation failed."
   exit 1
fi
SGE_INFOTEXT_MAX_COLUMN=5000; export SGE_INFOTEXT_MAX_COLUMN

#---------------------------------------
# setup INFOTEXT end
#---------------------------------------

ME=`whoami`
if [ "$ME" = "" ]; then
   $INFOTEXT "Can't determine your username with \"%s\" command. Exit." whoami
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

$INFOTEXT -auto $autoinst -ask "y" "n" -def "y" -n \
          "Do you want to start the update procedure(y/n) [y] >> "
if [ $? != 0 ]; then
   echo
   $INFOTEXT "Exiting update procedure. No changes were made."
   exit 1
fi

CheckForOldJobs
MakeBackupDirs

echo
$INFOTEXT "Deleting obsolete files..."

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
   $INFOTEXT "Deleting history directory tree: %s" $C_DIR/history
   Execute rm -rf $C_DIR/history
   Execute mkdir $C_DIR/history
fi

#-----------------------------------------------------------------
# DELETE: qsi directory tree
#
if [ -d $C_DIR/qsi ]; then
   $INFOTEXT "Deleting directory tree: %s" $C_DIR/qsi
   Execute rm -rf $C_DIR/qsi
fi

#-----------------------------------------------------------------
# DELETE: statistics file
#
if [ -f $C_DIR/statistics ]; then
   $INFOTEXT "Deleting file: %s" $C_DIR/statistics
   Execute rm $C_DIR/statistics
fi        

#-----------------------------------------------------------------
# DELETE: sched_runlog file
#
if [ -f $C_DIR/sched_runlog ]; then
   $INFOTEXT "Deleting file: %s" $C_DIR/sched_runlog
   Execute rm $C_DIR/sched_runlog
fi        

#-----------------------------------------------------------------
# DELETE: license file
#
if [ -f $C_DIR/license ]; then
   $INFOTEXT "Deleting file: %s" $C_DIR/license
   Execute rm $C_DIR/license
fi        

#-----------------------------------------------------------------
# RENAME: codine_aliases file
#
if [ -f $C_DIR/codine_aliases ]; then
   $INFOTEXT "Renaming file: %s" $C_DIR/codine_aliases
   Execute $MV $C_DIR/codine_aliases $C_DIR/sge_aliases
fi

#-----------------------------------------------------------------
# RENAME: cod_request file
#
if [ -f $C_DIR/cod_request ]; then
   $INFOTEXT "Renaming file: %s" $C_DIR/cod_request
   Execute $MV $C_DIR/cod_request $C_DIR/sge_request
fi

#-----------------------------------------------------------------
# UPGRADE: global cluster configuration
#
$INFOTEXT "Updating global cluster configuration file"

Execute $CP $C_DIR/configuration $BACKUP_DIR_COMMON
Execute $RM $C_DIR/configuration
Execute $TOUCH $C_DIR/configuration
$CMD_DIR/configuration.sh $SGE_MODE $BACKUP_DIR_COMMON/configuration $C_DIR/configuration \
                          $GID_RANGE "$SGE_COMPATIBILITY"
if [ $? != 0 ]; then
   $INFOTEXT "Failed updating cluster config: %s" $C_DIR/configuration
fi

#-----------------------------------------------------------------
# CREATE: settings.[c]sh
#
$INFOTEXT "Create new settings.[c]sh files"
Execute $CP $C_DIR/settings.sh $BACKUP_DIR_COMMON
Execute $CP $C_DIR/settings.csh $BACKUP_DIR_COMMON
$SGE_ROOT/util/create_settings.sh $C_DIR

#-----------------------------------------------------------------
# CREATE: product_mode file
#    be careful about the "-" combinations in this file, do not
#    destroy any additional settings in this file
#
$INFOTEXT "Creating new >%s< file" product_mode
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
      sed -e 's/sge/sgeee/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
   fi
elif [ $OLD_SGE_MODE = sge -a $SGE_MODE = sge ]; then
   sed -e 's/codine/sge/' $BACKUP_DIR_COMMON/product_mode > $C_DIR/product_mode
else
   $INFOTEXT "Ooops, this should not happen - please check your product_mode file"
   Execute $CP $BACKUP_DIR_COMMON/product_mode $C_DIR/product_mode
fi

#-----------------------------------------------------------------
# UPGRADE: accounting file
#
if [ -f $C_DIR/accounting ]; then
   $INFOTEXT "Updating accounting file"
   Execute $CP $C_DIR/accounting $BACKUP_DIR_COMMON
   Execute $RM $C_DIR/accounting
   env LC_ALL=C $AWK -f $CMD_DIR/accounting.awk $BACKUP_DIR_COMMON/accounting > $C_DIR/accounting
   if [ $? != 0 ]; then
      $INFOTEXT "Failed updating accounting file: %s" $C_DIR/accounting
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: scheduler configuration
#
if [ -f $C_DIR/sched_configuration ]; then
   $INFOTEXT "Updating scheduler configuration"
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
$INFOTEXT "Creating new startup script: %s" $C_DIR/rcsge
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
   $INFOTEXT "No queue directory found: %s" $MSPOOLdir/queues
else
   $INFOTEXT "Updating queues"

   if [ "`ls $MSPOOLdir/queues`" != "" ]; then
      Execute $CP $MSPOOLdir/queues/* $BACKUP_DIR_QUEUES
      Execute $RM -f $MSPOOLdir/queues/.??*
      Execute $RM -f $MSPOOLdir/queues/*
      
      for i in `ls $BACKUP_DIR_QUEUES`; do
         $INFOTEXT "   updating queue: %s" $i
         if [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
            env LC_ALL=C grep "^projects" $BACKUP_DIR_QUEUES/$i 2>&1 > /dev/null
            if [ $? = 0 ]; then
               $INFOTEXT "      queue is already updated - skipping"
               Execute cp $BACKUP_DIR_QUEUES/$i $MSPOOLdir/queues
            else
               Execute sed -f $CMD_DIR/sge2sgeee-queues.sed $BACKUP_DIR_QUEUES/$i > $MSPOOLdir/queues/$i
            fi   
         else
            env LC_ALL=C grep "^max_migr_time" $BACKUP_DIR_QUEUES/$i 2>&1 > /dev/null
            if [ $? != 0 ]; then
               $INFOTEXT "      queue is already updated - skipping"
               Execute cp $BACKUP_DIR_QUEUES/$i $MSPOOLdir/queues
            else
               Execute sed -f $CMD_DIR/queues.sed $BACKUP_DIR_QUEUES/$i > $MSPOOLdir/queues/$i
            fi   
         fi
      done
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: exec_hosts/
#
if [ ! -d $MSPOOLdir/exec_hosts ]; then
   $INFOTEXT "No queue directory found: %s" $MSPOOLdir/exec_hosts
else
   $INFOTEXT "Updating exec_hosts"

   if [ "`ls $MSPOOLdir/exec_hosts`" != "" ]; then
      Execute cp $MSPOOLdir/exec_hosts/* $BACKUP_DIR_EXEC_HOSTS
      Execute $RM -f $MSPOOLdir/exec_hosts/.??*
      Execute $RM -f $MSPOOLdir/exec_hosts/*
      
      for i in `ls $BACKUP_DIR_EXEC_HOSTS`; do
         $INFOTEXT "   updating exec host: %s" $i
         env LC_ALL=C grep "^real_host_name" $BACKUP_DIR_EXEC_HOSTS/$i 2>&1 > /dev/null
         if [ $? != 0 ]; then
            $INFOTEXT "      exec host is already updated - skipping"
            Execute cp $BACKUP_DIR_EXEC_HOSTS/$i $MSPOOLdir/exec_hosts
         fi
         if [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
            Execute sed -e '/^reschedule_unknown_list/d' -f $CMD_DIR/sge2sgeee-exec_hosts.sed $BACKUP_DIR_EXEC_HOSTS/$i > $MSPOOLdir/exec_hosts/$i
         else
            Execute sed -e '/^reschedule_unknown_list/d' -f $CMD_DIR/exec_hosts.sed $BACKUP_DIR_EXEC_HOSTS/$i > $MSPOOLdir/exec_hosts/$i
         fi
      done
   fi
fi

#-----------------------------------------------------------------
# UPGRADE: "queue" complex
#
if [ ! -d $MSPOOLdir/complexes ]; then
   $INFOTEXT "No complex directory found: %s" $MSPOOLdir/complexes
else
   $INFOTEXT "Updating >%s< complex" queue
   Execute cp $MSPOOLdir/complexes/queue $BACKUP_DIR_COMPLEXES
   sed -e "/^priority */d" $BACKUP_DIR_COMPLEXES/queue > $MSPOOLdir/complexes/queue
fi

#-----------------------------------------------------------------
# UPGRADE: usersets
#
if [ $OLD_SGE_MODE = sge -a $SGE_MODE = sgeee ]; then
   $INFOTEXT "Updating usersets"
   if [ "`ls $MSPOOLdir/usersets`" != "" ]; then

      Execute $CP $MSPOOLdir/usersets/* $BACKUP_DIR_USERSETS
      Execute $RM -f $MSPOOLdir/usersets/*
      Execute $RM -f $MSPOOLdir/usersets/.??*

      for i in `ls $BACKUP_DIR_USERSETS`; do
         $INFOTEXT "   updating userset: %s" $i
         env LC_ALL=C grep "^type" $BACKUP_DIR_USERSETS/$i 2>&1 > /dev/null
         if [ $? = 0 ]; then
            $INFOTEXT "      userset is already updated - skipping"
            Execute cp $BACKUP_DIR_USERSETS/$i $MSPOOLdir/usersets
         else
            Execute sed -f $CMD_DIR/usersets.sed $BACKUP_DIR_USERSETS/$i > $MSPOOLdir/usersets/$i
         fi
       done   
   fi

   if [ ! -f $MSPOOLdir/usersets/defaultdepartment ]; then
      $INFOTEXT "adding %s >%s< userset" SGEEE defaultdepartment
      Execute $CP $SGE_ROOT/util/resources/usersets/defaultdepartment $MSPOOLdir/usersets
   fi
   if [ ! -f $MSPOOLdir/usersets/deadlineusers ]; then
      $INFOTEXT "adding %s >%s< userset" SGEEE deadlineusers
      Execute $CP $SGE_ROOT/util/resources/usersets/deadlineusers $MSPOOLdir/usersets
   fi
fi

$INFOTEXT -wait -auto $autoinst -n "\nHit <RETURN> to continue >> "  
$CLEAR

echo
$INFOTEXT "A new Grid Engine startup script has been installed as"
$ECHO ""
$INFOTEXT "   %s" $C_DIR/rcsge
$ECHO ""
$INFOTEXT "This script may be used on all your hosts an all architectures for"
$INFOTEXT "starting Grid Engine."
$ECHO
$INFOTEXT "Please delete your old $QSYST startup files (not all of"
$INFOTEXT "of these files will exist on your systems):"
$ECHO ""
$INFOTEXT "     /etc/codine5_startup"        
$INFOTEXT "     /etc/init.d/codine5_startup"
$INFOTEXT "     /sbin/init.d/codine5_startup"
$INFOTEXT "     /etc/rcX.d/S95codine5"
$INFOTEXT "     /etc/grd_startup"
$INFOTEXT "     /etc/init.d/grd_startup"
$INFOTEXT "     /sbin/init.d/grd_startup"
$INFOTEXT "     /etc/rcX.d/S95grd"
$ECHO ""
$INFOTEXT "on all your hosts where Grid Engine is installed."

$INFOTEXT -wait -auto $autoinst -n "\nHit <RETURN> to continue >> "  
$CLEAR


$INFOTEXT "System wide installation of new Grid Engine startup script"
$ECHO "----------------------------------------------------------"
$ECHO
$INFOTEXT "It is recommended to install the new startup script"
$ECHO ""
$INFOTEXT "     $C_DIR/rcsge"
$ECHO ""
$INFOTEXT "on all your $QSYST hosts as"
$ECHO ""
$INFOTEXT "     /{etc|sbin}/init.d/rcsge"
$ECHO ""
$INFOTEXT "and create the necessary symbolic links in"
$ECHO ""
$INFOTEXT "     /{etc|sbin}/rcX.d/S95rcsge"

$INFOTEXT -wait -auto $autoinst -n "\nHit <RETURN> to continue >> "  
$CLEAR

$INFOTEXT "The Grid Engine update procedure has completed"
$ECHO "----------------------------------------------"
$ECHO "" 
$INFOTEXT "Please make sure to carry out all other steps as outlined in the file:"
$ECHO ""
$INFOTEXT "   %s" doc/UPGRADE
$ECHO ""
$INFOTEXT "of the Grid Engine distribution."

exit 0
