#!/bin/sh
#
# SGE/SGEEE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_common.sh
# Module: common functions
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

#-------------------------------------------------------------------------
#Setting up common variables and paths (eg. SGE_ARCH, utilbin, util)
#
BasicSettings()
{
  unset SGE_ND
  unset SGE_DEBUG_LEVEL

  SGE_UTIL="./util"
  SGE_ARCH=`$SGE_UTIL/arch`
  SGE_UTILBIN="./utilbin/$SGE_ARCH"
  SGE_BIN="./bin/$SGE_ARCH"


  shlib_path_name=`util/arch -lib`
  old_value=`eval echo '$'$shlib_path_name`
  if [ x$old_value = x ]; then
     eval $shlib_path_name=lib/$ARCH
  else
     eval $shlib_path_name=$old_value:lib/$ARCH
  fi
  export $shlib_path_name

  ADMINUSER=default
  MYUID=`$SGE_UTILBIN/uidgid -uid`
  MYGID=`$SGE_UTILBIN/uidgid -gid`
  DIRPERM=755
  FILEPERM=644

  SGE_MASTER_NAME=sge_qmaster
  SGE_EXECD_NAME=sge_execd
  SGE_SCHEDD_NAME=sge_schedd
  SGE_SHEPHERD_NAME=sge_shepherd
  SGE_SHADOWD_NAME=sge_shadowd
  SGE_SERVICE=sge_qmaster
  SGE_QMASTER_SRV=sge_qmaster
  SGE_EXECD_SRV=sge_execd

  unset SGE_NOMSG

  # set spooldefaults binary path
  SPOOLDEFAULTS=$SGE_UTILBIN/spooldefaults
  if [ ! -f $SPOOLDEFAULTS ]; then
     $ECHO "can't find \"$SPOOLDEFAULTS\""
     $ECHO "Installation failed."
     exit 1
  fi

  SPOOLINIT=$SGE_UTILBIN/spoolinit
  if [ ! -f $SPOOLINIT ]; then
     $ECHO "can't find \"$SPOOLINIT\""
     $ECHO "Installation failed."
     exit 1
  fi

  HOST=`$SGE_UTILBIN/gethostname -name`
  if [ "$HOST" = "" ]; then
     $INFOTEXT -e "can't get hostname of this machine. Installation failed."
     exit 1
  fi
}


#-------------------------------------------------------------------------
#Setting up INFOTEXT
#
SetUpInfoText()
{
# set column break for automatic line break
SGE_INFOTEXT_MAX_COLUMN=5000; export SGE_INFOTEXT_MAX_COLUMN


# export locale directory
if [ "$SGE_ROOT" = "" ]; then
   TMP_SGE_ROOT=`pwd | sed 's/\/tmp_mnt//'`
   GRIDLOCALEDIR="$TMP_SGE_ROOT/locale"
   export GRIDLOCALEDIR
fi

# set infotext binary path
INFOTXT_DUMMY=$SGE_UTILBIN/infotext
INFOTEXT=$INFOTXT_DUMMY
if [ ! -f $INFOTXT_DUMMY ]; then
   $ECHO "can't find \"$INFOTXT_DUMMY\""
   $ECHO "Installation failed."
   exit 1
fi
}

#-------------------------------------------------------------------------
# Enter: input is read and returned to stdout. If input is empty echo $1
#
# USES: variable "$AUTO"
#
Enter()
{
   if [ "$AUTO" = true ]; then
      $ECHO $1
   else
      read INP
      if [ "$INP" = "" ]; then
         $ECHO $1
      else
         $ECHO $INP
      fi
   fi
}


#-------------------------------------------------------------------------
# Makedir: make directory, chown/chgrp/chmod it. Exit if failure
#
Makedir()
{
   file=$1
   if [ ! -d $file ]; then
       $INFOTEXT "creating directory: %s" "$file"
       ExecuteAsAdmin $MKDIR -p $1
   fi

   ExecuteAsAdmin $CHMOD $DIRPERM $file
}

#-------------------------------------------------------------------------
# Execute command as user $ADMINUSER and exit if exit status != 0
# if ADMINUSER = default then execute command unchanged
#
# uses binary "adminrun" form SGE distribution
#
# USES: variables "$verbose"    (if set to "true" print arguments)
#                  $ADMINUSER   (if set to "default" do not use "adminrun)
#                 "$SGE_UTILBIN"  (path to the binary in utilbin)
#
ExecuteAsAdmin()
{
   if [ "$verbose" = true ]; then
      $ECHO $*
   fi

   if [ $ADMINUSER = default ]; then
      $*
   else
      $SGE_UTILBIN/adminrun $ADMINUSER $*
   fi

   if [ $? != 0 ]; then
      $ECHO >&2
      Translate 1 "Command failed: %s" "$*"
      $ECHO >&2
      Translate 1 "Probably a permission problem. Please check file access permissions."
      Translate 1 "Check read/write permission. Check if SGE daemons are running."
      $ECHO >&2
      exit 1
   fi
   return 0
}


#-------------------------------------------------------------------------
# SetPerm: set permissions
#
SetPerm()
{
   file=$1
   ExecuteAsAdmin $CHMOD $FILEPERM $file
}


#--------------------------------------------------------------------------
# SetCellDependentVariables
#
SetCellDependentVariables()
{
   COMMONDIR=$SGE_CELL_VAL/common
   LCONFDIR=$SGE_CELL_VAL/common/local_conf
   CASHAREDDIR=$COMMONDIR/sgeCA
}



#-------------------------------------------------------------------------
# CheckPath: Find and Remove Slash at the end of SGE_ROOT path
#
CheckPath()
{
   if [ "$SGE_ROOT" = "" ]; then
      SGE_ROOT=`pwd`
   fi
      MYTEMP=`echo $SGE_ROOT | sed 's/\/$//'`
      SGE_ROOT=$MYTEMP
#   export SGE_ROOT
}

#--------------------------------------------------------------------------
# CheckBinaries
#
CheckBinaries()
{

BINFILES="sge_coshepherd \
          sge_execd sge_qmaster  \
          sge_schedd sge_shadowd \
          sge_shepherd qacct qalter qconf qdel qhold \
          qhost qlogin qmake qmod qmon qresub qrls qrsh qselect qsh qstat qsub qtcsh"

UTILFILES="adminrun checkprog checkuser filestat gethostbyaddr gethostbyname \
           gethostname getservbyname loadcheck now qrsh_starter rlogin rsh rshd \
           testsuidroot uidgid infotext"

THIRD_PARTY_FILES=openssl

   missing=false
   for f in $BINFILES; do
      if [ ! -f $SGE_BIN/$f ]; then
         missing=true
         $INFOTEXT "missing program >%s< in directory >%s<" $f $SGE_BIN
         $INFOTEXT -log "missing program >%s< in directory >%s<" $f $SGE_BIN
      fi
   done

   #echo $CSP

   for f in $THIRD_PARTY_FILES; do
      if [ $f = openssl -a $CSP = true ]; then
         if [ ! -f $SGE_UTILBIN/$f ]; then
           missing=true
           $INFOTEXT "missing program >%s< in directory >%s<" $f $SGE_BIN
           $INFOTEXT -log "missing program >%s< in directory >%s<" $f $SGE_BIN

         fi
      fi
   done

   for f in $UTILFILES; do
      if [ ! -f $SGE_UTILBIN/$f ]; then
         missing=true
         $INFOTEXT "missing program >%s< in directory >%s<" $f $SGE_UTILBIN
         $INFOTEXT -log "missing program >%s< in directory >%s<" $f $SGE_UTILBIN
      fi
   done

   if [ $missing = true ]; then
      $INFOTEXT "\nMissing Grid Engine binaries!\n\n" \
      "A complete installation needs the following binaries in >%s<:\n\n" \
      "qacct           qlogin          qrsh            sge_shepherd\n" \
      "qalter          qmake           qselect         sge_coshepherd\n" \
      "qconf           qmod            qsh             sge_execd\n" \
      "qdel            qmon            qstat           sge_qmaster\n" \
      "qhold           qresub          qsub            sge_schedd\n" \
      "qhost           qrls            qtcsh           sge_shadowd\n\n" \
      "The binaries in >%s< are:\n\n" \
      "adminrun       gethostbyaddr  loadcheck      rlogin         uidgid\n" \
      "checkprog      gethostbyname  now            rsh            infotext\n" \
      "checkuser      gethostname    openssl        rshd\n" \
      "filestat       getservbyname  qrsh_starter   testsuidroot\n\n" \
      "Installation failed. Exit.\n" $SGE_BIN $SGE_UTILBIN

      $INFOTEXT -log "\nMissing Grid Engine binaries!\n\n" \
      "A complete installation needs the following binaries in >%s<:\n\n" \
      "qacct           qlogin          qrsh            sge_shepherd\n" \
      "qalter          qmake           qselect         sge_coshepherd\n" \
      "qconf           qmod            qsh             sge_execd\n" \
      "qdel            qmon            qstat           sge_qmaster\n" \
      "qhold           qresub          qsub            sge_schedd\n" \
      "qhost           qrls            qtcsh           sge_shadowd\n\n" \
      "The binaries in >%s< are:\n\n" \
      "adminrun       gethostbyaddr  loadcheck      rlogin         uidgid\n" \
      "checkprog      gethostbyname  now            rsh            infotext\n" \
      "checkuser      gethostname    openssl        rshd\n" \
      "filestat       getservbyname  qrsh_starter   testsuidroot\n\n" \
      "Installation failed. Exit.\n" $SGE_BIN $SGE_UTILBIN

      exit 1
   fi
}


#-------------------------------------------------------------------------
# ErrUsage: print usage string, exit
#
ErrUsage()
{
   myname=`basename $0`
   $ECHO >&2
   $INFOTEXT -e \
             "Usage: %s -m|-um|-x|-ux|-sm|-db [-auto filename ] [-csp]\n" \
             "              [-resport] [-afs] [-host] [-noremote]\n" \
             "   -m         install qmaster host\n" \
             "   -um        uninstall qmaster host\n" \
             "   -x         install execution host\n" \
             "   -ux        uninstall execution host\n" \
             "   -sm        install shadow host\n" \
             "   -db        install Berkeley DB on seperated Spooling Server\n" \
             "   -host      hostname for unistallation (eg. exec host)\n" \
             "   -auto      full automatic installation (qmaster and exec hosts)\n" \
             "   -csp       install system with security framework protocol\n" \
             "              functionality\n" \
             "   -afs       install system with AFS functionality\n" \
             "   -noremote  supress remote installation during autoinstall\n" \
             "   -help      show this help text\n\n" \
             "   Examples:\n" \
             "   inst_sge -m -x\n" \
             "                     Installs qmaster and exechost on localhost\n" \
             "   inst_sge -m -x -auto /path/to/config-file\n" \
             "                     Installs qmaster and exechost using the given\n" \
             "                     configuration file\n" \
             "                     (A templete can be found in:\n" \
             "                     util/install_modules/inst_sgeee_template.conf)\n" \
             "   inst_sge -ux -host hostname\n" \
             "                     Uninstalls execd on given executionhost\n" \
             "   inst_sge -db      Install a Berkeley DB Server on local host\n" \
             "   inst_sge -sm      Install a Shadow Master Host on local host" $myname 

   exit 1
}

#--------------------------------------------------------------------------
# Get SGE configuration from file (autoinstall mode)
#
GetConfigFromFile()
{
  IFS="
"
  if [ $FILE != "undef" ]; then
     $INFOTEXT -log "Reading configuration from file %s" $FILE
     . $FILE
  else
     $INFOTEXT -log "No config file. Please, start the installation with\n a valid configuration file"
  fi
  IFS="   
"
}


#--------------------------------------------------------------------------
#
WelcomeTheUser()
{
   if [ $AUTO = true ]; then
      return
   fi

   $INFOTEXT -u "\nWelcome to the Grid Engine installation"
   $INFOTEXT -u "\nGrid Engine qmaster host installation"
   $INFOTEXT "\nBefore you continue with the installation please read these hints:\n\n" \
             "   - Your terminal window should have a size of at least\n" \
             "     80x24 characters\n\n" \
             "   - The INTR character is often bound to the key Ctrl-C.\n" \
             "     The term >Ctrl-C< is used during the installation if you\n" \
             "     have the possibility to abort the installation\n\n" \
             "The qmaster installation procedure will take approximately 5-10 minutes.\n"
   $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
   $CLEAR
}


#-------------------------------------------------------------------------
# CheckWhoInstallsSGE
#
CheckWhoInstallsSGE()
{
   euid=`$SGE_UTILBIN/uidgid -euid`
   if [ $euid != 0 ]; then
      $CLEAR
      if [ $BERKELEY = "install" ]; then
         $INFOTEXT -u "\nBerkeley DB - test installation"
      else
         $INFOTEXT -u "\nGrid Engine - test installation"
      fi

      $INFOTEXT "\nYou are installing not as user >root<!\n\n" \
                  "This will allow you to run Grid Engine only under your user id for testing\n" \
                  "a limited functionality of Grid Engine.\n"

      ADMINUSER=`whoami`
      $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> if this is ok or stop the installation with Ctrl-C >> "
      $CLEAR
      return 0
   fi
   # from here only root
   this_dir_user=`$SGE_UTILBIN/filestat -owner . 2> /dev/null`
   ret=$?
   if [ $ret != 0 ]; then
      $INFOTEXT "\nCan't resolve user name from current directory.\n" \
                "Installation failed. Exit.\n"
      $INFOTEXT -log "\nCan't resolve user name from current directory.\n" \
                     "Installation failed. Exit.\n"
      exit 1
   fi

   if [ $this_dir_user != root ]; then
      $CLEAR
      $INFOTEXT -u "\nGrid Engine admin user account"

      $INFOTEXT "\nThe current directory\n\n" \
                "   %s\n\n" \
                "is owned by user\n\n" \
                "   %s\n\n" \
                "If user >root< does not have write permissions in this directory on *all*\n" \
                "of the machines where Grid Engine will be installed (NFS partitions not\n" \
                "exported for user >root< with read/write permissions) it is recommended to\n" \
                "install Grid Engine that all spool files will be created under the user id\n" \
                "of user >%s<.\n\n" \
                "IMPORTANT NOTE: The daemons still have to be started by user >root<.\n" \
                `pwd` $this_dir_user $this_dir_user

      $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n \
                "Do you want to install Grid Engine as admin user >%s< (y/n) [y] >> " $this_dir_user
      if [ $? = 0 ]; then
         $INFOTEXT "Installing Grid Engine as admin user >%s<" "$this_dir_user"
         $INFOTEXT -log "Installing Grid Engine as admin user >%s<" "$this_dir_user"
         ADMINUSER=$this_dir_user
         $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
         $CLEAR
         return
      else
         $CLEAR
      fi
   fi
   $INFOTEXT -u "\nChoosing Grid Engine admin user account"

   $INFOTEXT "\nYou may install Grid Engine that all files are created with the user id of an\n" \
             "unprivileged user.\n\n" \
             "This will make it possible to install and run Grid Engine in directories\n" \
             "where user >root< has no permissions to create and write files and directories.\n\n" \
             "   - Grid Engine still has to be started by user >root<\n\n" \
             "   - this directory should be owned by the Grid Engine administrator\n"

   $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n \
             "Do you want to install Grid Engine\n" \
             "under an user id other than >root< (y/n) [y] >> "

   if [ $? = 0 ]; then
      done=false
      while [ $done = false ]; do
         $CLEAR
         $INFOTEXT -u "\nChoosing a Grid Engine admin user name"
         $INFOTEXT -n "\nPlease enter a valid user name >> "
         INP=`Enter ""`
         if [ "$INP" != "" ]; then
            $SGE_UTILBIN/checkuser -check "$INP"
            if [ $? != 0 ]; then
               $INFOTEXT "User >%s< does not exist - please correct the username" $INP
               $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
               $CLEAR
            else
               $INFOTEXT "\nInstalling Grid Engine as admin user >%s<\n" $INP
               $INFOTEXT -log "Installing Grid Engine as user >%s<" $INP
               ADMINUSER=$INP
               if [ $ADMINUSER = root ]; then
                  ADMINUSER=default
               fi
               $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
               $CLEAR
               done=true
            fi
         fi
      done
   else
      $INFOTEXT "\nInstalling Grid Engine as user >root<\n"
      $INFOTEXT -log "Installing Grid Engine as user >root<"
      ADMINUSER=default
      $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
      $CLEAR
   fi
}

#-------------------------------------------------------------------------
# CheckForLocalHostResolving
#   "localhost", localhost.localdomain and 127.0.x.x are not supported
#   
#
CheckForLocalHostResolving()
{
   output=`$SGE_UTILBIN/gethostname| cut -f2 -d:`

   notok=false
   for cmp in $output; do
      case "$cmp" in
      localhost*|127.0*)
         notok=true
         ;;
      esac
   done

   if [ $notok = true ]; then
      $INFOTEXT -u "\nUnsupported local hostname"
      $INFOTEXT "\nThe current hostname is resolved as follows:\n\n"
      $SGE_UTILBIN/gethostname
      $INFOTEXT -wait -auto $AUTO -n \
                "\nIt is not supported for a Grid Engine installation that the local hostname\n" \
                "contains the hostname \"localhost\" and/or the IP address \"127.0.x.x\" of the\n" \
                "loopback interface.\n" \
                "The \"localhost\" hostname should be reserved for the loopback interface\n" \
                "(\"127.0.0.1\") and the real hostname should be assigned to one of the\n" \
                "physical or logical network interfaces of this machine.\n\n" \
                "Installation failed.\n\n" \
                "Press <RETURN> to exit the installation procedure >> "
      exit
   fi
}
               
#-------------------------------------------------------------------------
# ProcessSGERoot: read SGE/SGEEE root directory and set $SGE_ROOT
#                    check if $SGE_ROOT matches current directory
#
ProcessSGERoot()
{
   export SGE_ROOT

   done=false

   while [ $done = false ]; do
      if [ "$SGE_ROOT" = "" ]; then
         while [ "$SGE_ROOT" = "" ]; do
            $CLEAR
            $INFOTEXT -u "\nChecking \$SGE_ROOT directory"
            $ECHO
            $INFOTEXT -n "The Grid Engine root directory is not set!\n" \
                         "Please enter a correct path for SGE_ROOT or quit the installation\n" \
                         "with <CTRL-C> >> " 
         
            eval SGE_ROOT=`Enter`
         done
         export SGE_ROOT
      else
         $CLEAR
         $INFOTEXT -u "\nChecking \$SGE_ROOT directory"
         $ECHO
         eval SGE_ROOT=`pwd | sed 's/\/tmp_mnt//'`
         export SGE_ROOT
         SGE_ROOT_VAL=`eval echo $SGE_ROOT`

         $INFOTEXT -n "The Grid Engine root directory (your current directory) is:\n\n" \
                      "   \$SGE_ROOT = %s\n\n" \
                      "If this directory is not correct (e.g. it may contain an automounter\n" \
                      "prefix) enter the correct path to this directory or hit <RETURN>\n" \
                      "to use default [%s] >> " $SGE_ROOT_VAL $SGE_ROOT_VAL

         eval SGE_ROOT=`Enter $SGE_ROOT_VAL`
         $ECHO
      fi
      SGE_ROOT_VAL=`eval echo $SGE_ROOT`

      # do not check for correct SGE_ROOT in case of -nostrict
      if [ "$strict" = true ]; then
         # create a file in SGE_ROOT
         if [ $ADMINUSER != default ]; then
            $SGE_UTILBIN/adminrun $ADMINUSER $TOUCH $SGE_ROOT_VAL/tst$$ 2> /dev/null > /dev/null
         else
            touch $SGE_ROOT_VAL/tst$$ 2> /dev/null > /dev/null
         fi
         ret=$?
         # check if we have write permission
         if [ $ret != 0 ]; then
            $CLEAR
            $INFOTEXT "Can't create a temporary file in the directory\n\n   %s\n\n" \
                      "This may be a permission problem (e.g. no read/write permission\n" \
                      "on a NFS mounted filesystem).\n" \
                      "Please check your permissions. You may cancel the installation now\n" \
                      "and restart it or continue and try again.\n" $SGE_ROOT_VAL
            unset $SGE_ROOT
            $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
            $CLEAR
         elif [ ! -f tst$$ ]; then
            # check if SGE_ROOT points to current directory
            $INFOTEXT "Your \$SGE_ROOT environment variable\n\n   \$SGE_ROOT = %s\n\n" \
                        "doesn't match the current directory.\n" $SGE_ROOT_VAL
            ExecuteAsAdmin $RM -f $SGE_ROOT_VAL/tst$$
            unset $SGE_ROOT
         else
            ExecuteAsAdmin $RM -f $SGE_ROOT_VAL/tst$$
            done=true
         fi
      else
         done=true
      fi
   done

   $INFOTEXT "Your \$SGE_ROOT directory: %s\n" $SGE_ROOT_VAL
   $INFOTEXT -log "Your \$SGE_ROOT directory: %s" $SGE_ROOT_VAL
   $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
   $CLEAR
}

 
#-------------------------------------------------------------------------
# GiveHints: give some useful hints at the end of the installation
#
GiveHints()
{

   if [ $AUTO = true ]; then
      return
   fi

   done=false
   while [ $done = false ]; do
      $CLEAR
      $INFOTEXT -u "\nUsing Grid Engine"
      $INFOTEXT "\nYou should now enter the command:\n\n" \
                "   source %s\n\n" \
                "if you are a csh/tcsh user or\n\n" \
                "   # . %s\n\n" \
                "if you are a sh/ksh user.\n\n" \
                "This will set or expand the following environment variables:\n\n" \
                "   - \$SGE_ROOT         (always necessary)\n" \
                "   - \$SGE_CELL         (if you are using a cell other than >default<)\n" \
                "   - \$SGE_QMASTER_PORT (if you haven't added the service >sge_qmaster<)\n" \
                "   - \$SGE_EXECD_PORT   (if you haven't added the service >sge_execd<)\n" \
                "   - \$PATH/\$path       (to find the Grid Engine binaries)\n" \
                "   - \$MANPATH          (to access the manual pages)\n" \
                $SGE_ROOT_VAL/$SGE_CELL_VAL/common/settings.csh \
                $SGE_ROOT_VAL/$SGE_CELL_VAL/common/settings.sh

      $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to see where Grid Engine logs messages >> "
      $CLEAR

      tmp_spool=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep qmaster_spool_dir | awk '{ print $2 }'`
      master_spool=`dirname $tmp_spool`

      $INFOTEXT -u "\nGrid Engine messages"
      $INFOTEXT "\nGrid Engine messages can be found at:\n\n" \
                "   /tmp/qmaster_messages (during qmaster startup)\n" \
                "   /tmp/execd_messages   (during execution daemon startup)\n\n" \
                "After startup the daemons log their messages in their spool directories.\n\n" \
                "   Qmaster:     %s\n" \
                "   Exec daemon: <execd_spool_dir>/<hostname>/messages\n" $master_spool/messages

      $INFOTEXT -u "\nGrid Engine startup scripts"
      $INFOTEXT "\nGrid Engine startup scripts can be found at:\n\n" \
                "   %s (qmaster and scheduler)\n" \
                "   %s (execd)\n" $SGE_ROOT/$SGE_CELL/common/sgemaster $SGE_ROOT/$SGE_CELL/common/sgeexecd

      $INFOTEXT -auto $AUTO -ask "y" "n" -def "n" -n \
                "Do you want to see previous screen about using Grid Engine again (y/n) [n] >> "
      if [ $? = 0 ]; then
         :
      else
         done=true
      fi
   done

   if [ $QMASTER = install ]; then
      $CLEAR
      $INFOTEXT -u "\nYour Grid Engine qmaster installation is now completed"
      $INFOTEXT   "\nPlease now login to all hosts where you want to run an execution daemon\n" \
                  "and start the execution host installation procedure.\n\n" \
                  "If you want to run an execution daemon on this host, please do not forget\n" \
                  "to make the execution host installation in this host as well.\n\n" \
                  "All execution hosts must be administrative hosts during the installation.\n" \
                  "All hosts which you added to the list of administrative hosts during this\n" \
                  "installation procedure can now be installed.\n\n" \
                  "You may verify your administrative hosts with the command\n\n" \
                  "   # qconf -sh\n\n" \
                  "and you may add new administrative hosts with the command\n\n" \
                  "   # qconf -ah <hostname>\n\n"
       $INFOTEXT -wait -n "Please hit <RETURN> >> "
       $CLEAR
       QMASTER="undef"
      return 0
   else
      $INFOTEXT "Your execution daemon installation is now completed."
   fi
}


#-------------------------------------------------------------------------
# PrintLocalConf:  print execution host local SGE/SGEEE configuration
#
PrintLocalConf()
{

   arg=$1
   if [ $arg = 1 ]; then
      $ECHO "# Version: pre6.0"
      $ECHO "#"
      $ECHO "# DO NOT MODIFY THIS FILE MANUALLY!"
      $ECHO "#"
      $ECHO "conf_version           0"
   fi
   $ECHO "mailer                 $MAILER"
   $ECHO "xterm                  $XTERM"
   $ECHO "qlogin_daemon          $QLOGIN_DAEMON"
   $ECHO "rlogin_daemon          $RLOGIN_DAEMON"
   if [ $LOCAL_EXECD_SPOOL != "undef" ]; then
      $ECHO "execd_spool_dir        $LOCAL_EXECD_SPOOL"
   fi
}


#-------------------------------------------------------------------------
# CreateSGEStartUpScripts: create startup scripts 
#
CreateSGEStartUpScripts()
{
   euid=$1
   create=$2
   hosttype=$3

   if [ $hosttype = "master" ]; then
      TMP_SGE_STARTUP_FILE=/tmp/sgemaster.$$
      STARTUP_FILE_NAME=sgemaster
      S95NAME=S95sgemaster
   else
      TMP_SGE_STARTUP_FILE=/tmp/sgeexecd.$$
      STARTUP_FILE_NAME=sgeexecd
      S95NAME=S95sgeexecd
   fi

   if [ -f $TMP_SGE_STARTUP_FILE ]; then
      Execute rm $TMP_SGE_STARTUP_FILE
   fi
   if [ -f ${TMP_SGE_STARTUP_FILE}.0 ]; then
      Execute rm ${TMP_SGE_STARTUP_FILE}.0
   fi
   if [ -f ${TMP_SGE_STARTUP_FILE}.1 ]; then
      Execute rm ${TMP_SGE_STARTUP_FILE}.1
   fi

   SGE_STARTUP_FILE=$SGE_ROOT_VAL/$COMMONDIR/$STARTUP_FILE_NAME

   if [ $create = true ]; then

      if [ $hosttype = "master" ]; then
         Execute sed -e "s%GENROOT%${SGE_ROOT_VAL}%g" \
                     -e "s%GENCELL%${SGE_CELL_VAL}%g" \
                     -e "/#+-#+-#+-#-/,/#-#-#-#-#-#/d" \
                     util/rctemplates/sgemaster_template > ${TMP_SGE_STARTUP_FILE}.0

         if [ "$SGE_QMASTER_PORT" != "" ]; then
            Execute sed -e "s/=GENSGE_QMASTER_PORT/=$SGE_QMASTER_PORT/" \
                        ${TMP_SGE_STARTUP_FILE}.0 > $TMP_SGE_STARTUP_FILE.1
         else
            Execute sed -e "/GENSGE_QMASTER_PORT/d" \
                        ${TMP_SGE_STARTUP_FILE}.0 > $TMP_SGE_STARTUP_FILE.1
         fi

         if [ "$SGE_EXECD_PORT" != "" ]; then
            Execute sed -e "s/=GENSGE_EXECD_PORT/=$SGE_EXECD_PORT/" \
                        ${TMP_SGE_STARTUP_FILE}.1 > $TMP_SGE_STARTUP_FILE
         else
            Execute sed -e "/GENSGE_EXECD_PORT/d" \
                        ${TMP_SGE_STARTUP_FILE}.1 > $TMP_SGE_STARTUP_FILE
         fi
      else
         Execute sed -e "s%GENROOT%${SGE_ROOT_VAL}%g" \
                     -e "s%GENCELL%${SGE_CELL_VAL}%g" \
                     -e "/#+-#+-#+-#-/,/#-#-#-#-#-#/d" \
                     util/rctemplates/sgeexecd_template > ${TMP_SGE_STARTUP_FILE}.0

         if [ "$SGE_QMASTER_PORT" != "" ]; then
            Execute sed -e "s/=GENSGE_QMASTER_PORT/=$SGE_QMASTER_PORT/" \
                        ${TMP_SGE_STARTUP_FILE}.0 > $TMP_SGE_STARTUP_FILE.1
         else
            Execute sed -e "/GENSGE_QMASTER_PORT/d" \
                        ${TMP_SGE_STARTUP_FILE}.0 > $TMP_SGE_STARTUP_FILE.1
         fi

         if [ "$SGE_EXECD_PORT" != "" ]; then
            Execute sed -e "s/=GENSGE_EXECD_PORT/=$SGE_EXECD_PORT/" \
                        ${TMP_SGE_STARTUP_FILE}.1 > $TMP_SGE_STARTUP_FILE
         else
            Execute sed -e "/GENSGE_EXECD_PORT/d" \
                        ${TMP_SGE_STARTUP_FILE}.1 > $TMP_SGE_STARTUP_FILE
         fi

       fi

      ExecuteAsAdmin $CP $TMP_SGE_STARTUP_FILE $SGE_STARTUP_FILE
      ExecuteAsAdmin $CHMOD a+x $SGE_STARTUP_FILE

      rm -f $TMP_SGE_STARTUP_FILE ${TMP_SGE_STARTUP_FILE}.0 ${TMP_SGE_STARTUP_FILE}.1

      if [ $euid = 0 -a $ADMINUSER != default -a $QMASTER = "install" -a $hosttype = "master" ]; then
         AddDefaultManager root $ADMINUSER
         AddDefaultOperator $ADMINUSER
      elif [ $euid != 0 -a $hosttype = "master" ]; then
         AddDefaultManager $USER
         AddDefaultOperator $USER
      fi

      $INFOTEXT "Creating >%s< script" $STARTUP_FILE_NAME 
   fi

}



#-------------------------------------------------------------------------
# AddSGEStartUpScript: Add startup script to rc files if root installs
#
AddSGEStartUpScript()
{
   euid=$1
   hosttype=$2

   $CLEAR
   if [ $hosttype = "master" ]; then
      TMP_SGE_STARTUP_FILE=/tmp/sgemaster.$$
      STARTUP_FILE_NAME=sgemaster
      S95NAME=S95sgemaster
      DAEMON_NAME="qmaster/scheduler"
   else
      TMP_SGE_STARTUP_FILE=/tmp/sgeexecd.$$
      STARTUP_FILE_NAME=sgeexecd
      S95NAME=S95sgeexecd
      DAEMON_NAME="execd"
   fi

   SGE_STARTUP_FILE=$SGE_ROOT_VAL/$COMMONDIR/$STARTUP_FILE_NAME


   if [ $euid != 0 ]; then
      return 0
   fi

   $INFOTEXT -u "\n%s startup script" $DAEMON_NAME

   # --- from here only if root installs ---
   $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n \
             "\nWe can install the startup script that will\n" \
             "start %s at machine boot (y/n) [y] >> " $DAEMON_NAME

   ret=$?
   if [ $AUTO = "true" -a $ADD_TO_RC = "false" ]; then
      $CLEAR
      return
   else
      if [ $ret = 1 ]; then
         $CLEAR
         return
      fi
   fi

   # If we have System V we need to put the startup script to $RC_PREFIX/init.d
   # and make a link in $RC_PREFIX/rc2.d to $RC_PREFIX/init.d
   if [ "$RC_FILE" = "sysv_rc" ]; then
      $INFOTEXT "Installing startup script %s" "$RC_PREFIX/$RC_DIR/$S95NAME"
      Execute rm -f $RC_PREFIX/$RC_DIR/$S95NAME
      Execute cp $SGE_STARTUP_FILE $RC_PREFIX/init.d/$STARTUP_FILE_NAME
      Execute chmod a+x $RC_PREFIX/init.d/$STARTUP_FILE_NAME
      Execute ln -s $RC_PREFIX/init.d/$STARTUP_FILE_NAME $RC_PREFIX/$RC_DIR/$S95NAME

      # runlevel management in Linux is different -
      # each runlevel contains full set of links
      # RedHat uses runlevel 5 and SUSE runlevel 3 for xdm
      # RedHat uses runlevel 3 for full networked mode
      # Suse uses runlevel 2 for full networked mode
      # we already installed the script in level 3
      SGE_ARCH=`$SGE_UTIL/arch`
      case $SGE_ARCH in
      lx2?-*)
         runlevel=`grep "^id:.:initdefault:"  /etc/inittab | cut -f2 -d:`
         if [ "$runlevel" = 2 -o  "$runlevel" = 5 ]; then
            $INFOTEXT "Installing startup script also in %s" "$RC_PREFIX/rc${runlevel}.d/$S95NAME"
            Execute rm -f $RC_PREFIX/rc${runlevel}.d/$S95NAME
            Execute ln -s $RC_PREFIX/init.d/$STARTUP_FILE_NAME $RC_PREFIX/rc${runlevel}.d/$S95NAME
         fi
         ;;
       esac

   elif [ "$RC_FILE" = "insserv-linux" ]; then
      echo  cp $SGE_STARTUP_FILE $RC_PREFIX/$STARTUP_FILE_NAME
      echo /sbin/insserv $RC_PREFIX/$STARTUP_FILE_NAME
      Execute cp $SGE_STARTUP_FILE $RC_PREFIX/$STARTUP_FILE_NAME
      /sbin/insserv $RC_PREFIX/$STARTUP_FILE_NAME
   elif [ "$RC_FILE" = "freebsd" ]; then
      echo  cp $SGE_STARTUP_FILE $RC_PREFIX/sge${RC_SUFFIX}
      Execute cp $SGE_STARTUP_FILE $RC_PREFIX/sge${RC_SUFFIX}
   else
      # if this is not System V we simple add the call to the
      # startup script to RC_FILE

      # Start-up script already installed?
      #------------------------------------
      grep $STARTUP_FILE_NAME $RC_FILE > /dev/null 2>&1
      status=$?
      if [ $status != 0 ]; then
         $INFOTEXT "Adding application startup to %s" $RC_FILE
         # Add the procedure
         #------------------
         $ECHO "" >> $RC_FILE
         $ECHO "" >> $RC_FILE
         $ECHO "# Grid Engine start up" >> $RC_FILE
         $ECHO "#-$LINE---------" >> $RC_FILE
         $ECHO $SGE_STARTUP_FILE >> $RC_FILE
      else
         $INFOTEXT "Found a call of %s in %s. Replacing with new call.\n" \
                   "Your old file %s is saved as %s" $STARTUP_FILE_NAME $RC_FILE $RC_FILE $RC_FILE.org.1

         mv $RC_FILE.org.3 $RC_FILE.org.4    2>/dev/null
         mv $RC_FILE.org.2 $RC_FILE.org.3    2>/dev/null
         mv $RC_FILE.org.1 $RC_FILE.org.2    2>/dev/null

         # save original file modes of RC_FILE
         uid=`$SGE_UTILBIN/filestat -uid $RC_FILE`
         gid=`$SGE_UTILBIN/filestat -gid $RC_FILE`
         perm=`$SGE_UTILBIN/filestat -mode $RC_FILE`

         Execute cp $RC_FILE $RC_FILE.org.1

         savedfile=`basename $RC_FILE`

         sed -e "s%.*$STARTUP_FILE_NAME.*%$SGE_STARTUP_FILE%" \
                 $RC_FILE > /tmp/$savedfile.1

         Execute cp /tmp/$savedfile.1 $RC_FILE
         Execute chown $uid $RC_FILE
         Execute chgrp $gid $RC_FILE
         Execute chmod $perm $RC_FILE
         Execute rm -f /tmp/$savedfile.1
      fi
   fi

   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
   $CLEAR
}


#-------------------------------------------------------------------------
# AddDefaultManager
#
AddDefaultManager()
{
   ExecuteAsAdmin $SPOOLDEFAULTS managers $*
#  TruncCreateAndMakeWriteable $QMDIR/managers
#  $ECHO $1 >> $QMDIR/managers
#  SetPerm $QMDIR/managers
}


#-------------------------------------------------------------------------
# AddDefaultOperator
#
AddDefaultOperator()
{
   ExecuteAsAdmin $SPOOLDEFAULTS operators $*
}

MoveLog()
{
   if [ $EXECD = "uninstall" ]; then
      cp /tmp/$LOGNAME $SGE_ROOT/$SGE_CELL/common/uninstall$DATE.log 2>&1
   else
      cp /tmp/$LOGNAME $SGE_ROOT/$SGE_CELL/common/install$DATE.log 2>&1
   fi

   rm /tmp/$LOGNAME 2>&1
}

CreateLog()
{
LOGNAME=install.$$
DATE=`date '+%Y-%m-%d_%H:%M:%S'`

if [ -f /tmp/$LOGNAME ]; then
   rm /tmp/$LOGNAME
   touch /tmp/$LOGNAME
else
   touch /tmp/$LOGNAME
fi
}

