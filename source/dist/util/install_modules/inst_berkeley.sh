#! /bin/sh
#
# SGE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_berkeley.sh
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
   if [ "$BERKELEY" = "install" ]; then
      $INFOTEXT -u "\nBerkeley Database spooling parameters"
      $INFOTEXT "\nYou are going to install a RPC Client/Server machanism!" \
                "\nIn this case, qmaster will" \
                "\ncontact a RPC server running on a separate server machine." \
                "\nIf you want to use the SGE shadowd, you have to use the " \
                "\nRPC Client/Server mechanism.\n"
                SPOOLING_SERVER=`hostname`
   $INFOTEXT -n "\nEnter database server name or \nhit <RETURN> to use default [%s] >> " $SPOOLING_SERVER
                SPOOLING_SERVER=`Enter $SPOOLING_SERVER`

   $INFOTEXT -n "\nEnter the database directory\n" \
                "or hit <RETURN> to use default [%s] >> " "$SGE_ROOT/$SGE_CELL/$SPOOLING_DIR"
                SPOOLING_DIR=`Enter $SGE_ROOT/$SGE_CELL/$SPOOLING_DIR`
   else
     $INFOTEXT -u "\nBerkeley Database spooling parameters"

     if [ "$is_server" = "true" ]; then
        $INFOTEXT -n "\nPlease enter the name of your Berkeley DB Spooling Server! >> "
               SPOOLING_SERVER=`Enter $SPOOLING_SERVER`
        $INFOTEXT -n "Please enter the Database Directory now!\n"
        $INFOTEXT -n "Default: [%s] >> " "$SGE_ROOT/$SGE_CELL/spooldb"
        SPOOLING_DIR="$SGE_ROOT/$SGE_CELL/spooldb" 
        SPOOLING_DIR=`Enter $SPOOLING_DIR`
     else
        SPOOLING_SERVER=none
        $INFOTEXT -n "\nPlease enter the Database Directory now, even if you want to spool locally,\n" \
                     "it is necessary to enter this Database Directory. \n\nDefault: [%s] >> " `dirname $QMDIR`"/spooldb" 
                  SPOOLING_DIR=`dirname $QMDIR`"/spooldb" 
                  SPOOLING_DIR=`Enter $SPOOLING_DIR`        
     fi

     if [ "$AUTO" = "true" ]; then
        SPOOLING_DIR=$DB_SPOOLING_DIR
     fi
 
   fi
}

SpoolingCheckParams()
{
   # if we use local spooling, check if the database directory is on local fs
   if [ "$SPOOLING_SERVER" = "none" ]; then
      CheckLocalFilesystem $SPOOLING_DIR
      ret=$?
      if [ $ret -eq 0 ]; then
      $INFOTEXT -e "\nThe database directory >%s<\n" \
                   "is not on a local filesystem.\nPlease choose a local filesystem or configure the RPC Client/Server mechanism" $SPOOLING_DIR
      if [ "$AUTO" = "true" ]; then
         $INFOTEXT -log "\nThe database directory >%s<\n" \
                   "is not on a local filesystem.\nPlease choose a local filesystem or configure the RPC Client/Server mechanism" $SPOOLING_DIR
         MoveLog
         exit 1
      fi
         return 0
      else
         return 1
      fi
   else 
      # TODO: we should check if the hostname can be resolved
      # create a script to start the rpc server
      Makedir $SPOOLING_DIR

      # Deactivated the copy of DB_CONFIG file. The DB_CONFIG file is still distributed 
      #DB_CONFIG_COPY="cp ./util/install_modules/DB_CONFIG $SPOOLING_DIR/DB_CONFIG"
      #ExecuteAsAdmin $DB_CONFIG_COPY
      CreateRPCServerScript
      $INFOTEXT "\nNow we have to startup the rc script\n >%s< \non the RPC server machine\n" $SGE_ROOT/$COMMONDIR/sgebdb
      $INFOTEXT -n "If you already have a configured Berkeley DB Spooling Server,\n you have to restart "
      $INFOTEXT "the Database with the rc script now and continue with >NO<\n"
      $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "Shall the installation script try to start the RPC server? (y/n) [y] >>"

      if [ $? = 0 ]; then
         $INFOTEXT -log "Starting rpc server on host %s!" $SPOOLING_SERVER
         $INFOTEXT "Starting rpc server on host %s!" $SPOOLING_SERVER
         ExecuteRPCServerScript start
         sleep 5
         $INFOTEXT "The Berkeley DB has been started with these parameters:\n\n"
         $INFOTEXT "Spooling Server Name: %s" $SPOOLING_SERVER
         $INFOTEXT "DB Spooling Directory: %s\n" $SPOOLING_DIR
         $INFOTEXT -wait -auto $AUTO -n "Please remember these values, during Qmaster installation\n you will be asked for! Hit <RETURN> to continue!"
      else
         $INFOTEXT "Please start the rc script \n>%s< on the RPC server machine\n" $SGE_ROOT/$COMMONDIR/sgebdb
         $INFOTEXT "If your database is already running, then continue with <RETURN>\n"
         $INFOTEXT -auto $AUTO -wait -n "Hit <RETURN> to continue >>"
      fi

      $INFOTEXT "The Berkeley DB installation is completed now!"

   return 1
   fi
}

CreateRPCServerScript()
{
   pid=$$
   TMP_DIR=/tmp/$pid
   TMP_RC=/tmp/$pid/sgebdb
   RPCSCRIPT=$SGE_ROOT/$COMMONDIR/sgebdb
   ExecuteAsAdmin mkdir -p $TMP_DIR
   ExecuteAsAdmin sed -e "s%GENROOT%${SGE_ROOT_VAL}%g" \
                      -e "s%GENCELL%${SGE_CELL_VAL}%g" \
                      -e "s%GENADMINUSER%${ADMINUSER}%g" \
                      -e "s%SPOOLING_DIR%${SPOOLING_DIR}%g" \
                      -e "/#+-#+-#+-#-/,/#-#-#-#-#-#/d" \
                      util/rctemplates/sgebdb_template > ${TMP_RC}
   ExecuteAsAdmin cp $TMP_RC $RPCSCRIPT
   ExecuteAsAdmin rm -fR $TMP_DIR 
   ExecuteAsAdmin $CHMOD a+x $RPCSCRIPT
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

   if [ `$SGE_UTILBIN/fstype $FS` = "nfs4" ]; then
      return 1
   elif [ `$SGE_UTILBIN/fstype $FS | grep "nfs" | wc -l` -gt 0 ]; then
      return 0
   else
      return 1
   fi

}

# Executes the RPC Startup Script
# $1 is start or stop
# The Script is either executed on the local host or on a remote host
ExecuteRPCServerScript()
{
   ExecuteAsAdmin $SGE_ROOT/$SGE_CELL/common/sgebdb $1
}

DeleteSpoolingDir()
{
   QMDIR="$SGE_ROOT/$SGE_CELL/qmaster"
   SpoolingQueryChange  

   ExecuteAsAdmin rm -fr $SPOOLING_DIR
   
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

