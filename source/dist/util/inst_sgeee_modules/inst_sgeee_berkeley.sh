#! /bin/sh
#
# SGE/SGEEE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_sgeee_berkeley.sh
# Module: berkeley db install functions
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


SpoolingQueryChange()
{
   if [ $BERKELEY = "install" ]; then
      $INFOTEXT -u "\nBerkeley Database spooling parameters"
      $INFOTEXT "\nYou are going to install a RPC Client/Server machanism!" \
                "\nIn this case, qmaster will" \
                "\ncontact a RPC server running on a separate server machine." \
                "\nIf you want to use the SGE shadowd, you have to use the " \
                "\nRPC Client/Server mechanism.\n"
                SPOOLING_SERVER=`hostname`
      $INFOTEXT "\nEnter database server name or \nhit <RETURN> to use default [%s] >> " $SPOOLING_SERVER
                SPOOLING_SERVER=`Enter $SPOOLING_SERVER`

      $INFOTEXT "\nEnter the database directory\n" \
                "or hit <RETURN> to use default [%s] >> " "$SGE_ROOT/$SGE_CELL/$SPOOLING_DIR"
                SPOOLING_DIR=`Enter $SGE_ROOT/$SGE_CELL/$SPOOLING_DIR`
   else
     $INFOTEXT -u "\nBerkeley Database spooling parameters"

     if [ $is_server = "true" ]; then
        $INFOTEXT -n "Please enter the name of your Berkeley DB Spooling Server! >> "
               SPOOLING_SERVER=`Enter`
        $INFOTEXT -n "Please enter the Database Directory now!\n"
        $INFOTEXT -n "Default: [%s] >> " "$SGE_ROOT/$SGE_CELL/spooldb"
        SPOOLING_DIR="$SGE_ROOT/$SGE_CELL/spooldb" 
        SPOOLING_DIR=`Enter $SPOOLING_DIR`
     else
        SPOOLING_SERVER=none
        $INFOTEXT -n "Please enter the Database Directory now, even if you want to spool locally\n" \
                     "it is necessary to enter this Database Directory. \nDefault: [%s] >> " "$SGE_ROOT/$SGE_CELL/spooldb"
                  SPOOLING_DIR="$SGE_ROOT/$SGE_CELL/spooldb" 
                  SPOOLING_DIR=`Enter $SPOOLING_DIR`        
     fi
 
   fi
 
}

SpoolingCheckParams()
{
   # if we use local spooling, check if the database directory is on local fs
   if [ $SPOOLING_SERVER = "none" ]; then
      CheckLocalFilesystem $SPOOLING_DIR
      ret=$?
      if [ $ret -eq 0 ]; then
      $INFOTEXT -e "\nThe database directory >%s<\n" \
                   "is not on a local filesystem.\nPlease choose a local filesystem or configure the RPC Client/Server mechanism" $SPOOLING_DIR
         return 0
      else
         return 1
      fi
   else 
      # TODO: we should check if the hostname can be resolved
      # create a script to start the rpc server
      Makedir $SPOOLING_DIR
      ExecuteAsAdmin `cp ./util/inst_sgeee_modules/DB_CONFIG $SPOOLING_DIR/DB_CONFIG`
      CreateRPCServerScript
      $INFOTEXT "\nNow we have to startup the rc script\n >%s< \non the RPC server machine\n" $SGE_ROOT/$COMMONDIR/sgebdb
      $INFOTEXT -n "If you already have a configured Berkeley DB Spooling Server,\n you have to restart "
      $INFOTEXT "the Database with the rc script now and continue with >NO<\n"
      $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "Shall the installation script try to start the RPC server? (y/n) [y] >>"

      if [ $? = 0 -a $AUTO = "false" ]; then
         $INFOTEXT -log "Starting rpc server on host %s!" $SPOOLING_SERVER
         $INFOTEXT "Starting rpc server on host %s!" $SPOOLING_SERVER
         exec $SGE_ROOT/$COMMONDIR/sgebdb start &
         sleep 5
         $INFOTEXT "The Berkely DB has been started with these parameters:\n\n"
         $INFOTEXT "Spooling Server Name: %s" $SPOOLING_SERVER
         $INFOTEXT "DB Spooling Directory: %s\n" $SPOOLING_DIR
         $INFOTEXT -wait "Please remember these values, during Qmaster installation\n you will be asked for! Hit <RETURN> to continue!"
      else
         $INFOTEXT "Please start the rc script \n>%s< on the RPC server machine\n" $SGE_ROOT/$COMMONDIR/sgebdb
         $INFOTEXT "If your database is already running, then continue with <RETURN>\n"
         $INFOTEXT -auto $AUTO -wait "Hit <RETURN> to continue >>"
      fi

      $INFOTEXT "The Berkely DB installation is completed now!"

   return 1
   fi
}

CreateRPCServerScript()
{
   RPCSCRIPT=$SGE_ROOT/$COMMONDIR/sgebdb
   Execute sed -e "s%GENROOT%${SGE_ROOT_VAL}%g" \
               -e "s%GENCELL%${SGE_CELL_VAL}%g" \
               -e "s%SPOOLING_DIR%${SPOOLING_DIR}%g" \
               -e "/#+-#+-#+-#-/,/#-#-#-#-#-#/d" \
               util/rpc_startup_template > ${RPCSCRIPT}
   Execute $CHMOD a+x $RPCSCRIPT

}


CheckLocalFilesystem()
{
   is_done="false"
   FS=$1

   while [ $is_done = "false" ]; do  
      FS=`dirname $FS`
      if [ -d $FS ]; then
         is_done="true"
      fi
   done

   case $ARCH in
      sol*)
         df -l $FS >/dev/null 2>&1
         if [ $? -eq 0 ]; then
            return 1
         else
            return 0
         fi
         ;;
      lx*)
         df -l $FS >/dev/null 2>&1
         if [ $? -eq 0 ]; then
            return 1
         else
            return 0
         fi
         ;;
      *)
         $INFOTEXT -e "\nDon't know how to test for local filesystem. Exit."
         $INFOTEXT -wait -n "\nPlease make sure that the directory $FS is on a local filesystem!\nHit <RETURN> to continue >> "
         exit 1
         ;;
   esac

   return 0
}


InstallServerScript()
{
   euid=$1

   $CLEAR
      STARTUP_FILE_NAME=sgebdb
      S95NAME=S95sgebdb

   SGE_STARTUP_FILE=$SGE_ROOT_VAL/$COMMONDIR/$STARTUP_FILE_NAME


   if [ $euid != 0 ]; then
      return 0
   fi

   $INFOTEXT -u "\nBerkeley DB startup script"

   # --- from here only if root installs ---
   $INFOTEXT -auto $AUTO -ask "y" "n" -def "n" -n \
             "\nWe can install the startup script that\n" \
             "Grid Engine is started at machine boot (y/n) [n] >> "

   if [ $AUTO = "true" -a $ADD_TO_RC = "false" ]; then
      $CLEAR
      return
   else
      if [ $? = 1 ]; then
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
      if [ $ARCH = linux -o $ARCH = glinux -o $ARCH = alinux -o $ARCH = slinux ]; then
         runlevel=`grep "^id:.:initdefault:"  /etc/inittab | cut -f2 -d:`
         if [ "$runlevel" = 2 -o  "$runlevel" = 5 ]; then
            $INFOTEXT "Installing startup script also in %s" "$RC_PREFIX/rc${runlevel}.d/$S95NAME"
            Execute rm -f $RC_PREFIX/rc${runlevel}.d/$S95NAME
            Execute ln -s $RC_PREFIX/init.d/$STARTUP_FILE_NAME $RC_PREFIX/rc${runlevel}.d/$S95NAME
         fi
      fi
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
         $ECHO "# Berkeley DB start up" >> $RC_FILE
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

EditStartupScript()
{
 TMP_BDBHOME=`cat $TMP_STARTUP_SCRIPT | grep "^[ \t]*BDBHOMES" | cut -d"=" -f2 | sed s/\"//g | cut -d" " -f2`
 BDBHOME=$SPOOLING_DIR
 BDBHOMES=" -h "$TMP_BDBHOME" -h "$BDBHOME


 cat $TMP_STARTUP_SCRIPT | sed -e s§\"$TMP_BDBHOME\"§\"$BDBHOMES\"§g > $TMP_STARTUP_SCRIPT.0
 `cp $TMP_STARTUP_SCRIPT.0 $TMP_STARTUP_SCRIPT`
 rm $TMP_STARTUP_SCRIPT.0


}

