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
   if [ -z "$1" ]; then
      SPOOLING_DIR=$SGE_ROOT/$SGE_CELL/spooldb
   else
      SPOOLING_DIR="$1"
   fi

   if [ -f "$SGE_ROOT/$SGE_CELL/common/bootstrap" ]; then
      ignore_fqdn=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "ignore_fqdn" | awk '{ print $2 }'`
      default_domain=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "default_domain" | awk '{ print $2 }'`
   else
      if [ "$IGNORE_FQDN_DEFAULT" != "true" -a "$IGNORE_FQDN_DEFAULT" != "false" ]; then
         SelectHostNameResolving
      fi      
      ignore_fqdn=$IGNORE_FQDN_DEFAULT
      default_domain=$CFG_DEFAULT_DOMAIN
   fi

   if [ "$BERKELEY" = "install" ]; then
      $INFOTEXT -u "\nBerkeley Database spooling parameters"
      $INFOTEXT "\nYou are going to install a RPC Client/Server mechanism!" \
                "\nIn this case, qmaster will" \
                "\ncontact a RPC server running on a separate server machine." \
                "\nIf you want to use the SGE shadowd, you have to use the " \
                "\nRPC Client/Server mechanism.\n"
      SPOOLING_SERVER=`$SGE_UTILBIN/gethostname -aname`
      $INFOTEXT -n "\nEnter database server name or \nhit <RETURN> to use default [%s] >> " $SPOOLING_SERVER
      SPOOLING_SERVER=`Enter $SPOOLING_SERVER`

      $INFOTEXT -n "\nEnter the database directory\n" \
                   "or hit <RETURN> to use default [%s] >> " $SPOOLING_DIR
      SPOOLING_DIR=`Enter $SPOOLING_DIR`
   else
     $INFOTEXT -u "\nBerkeley Database spooling parameters"

     if [ "$is_server" = "true" ]; then
        #TODO: Does not work is_server is not set
        if [ -n "$1" -a -n "$2" ]; then
           default_host="$1"
           default_spool_dir="$2"
        else
           default_host="$HOST"
           default_spool_dir="$SGE_ROOT/$SGE_CELL/spooldb"
        fi

        $INFOTEXT -n "\nEnter the name of your Berkeley DB Spooling Server [%s] >> " "$default_host"
        SPOOLING_SERVER=`Enter $SPOOLING_SERVER`
        $INFOTEXT -n "Enter the database directory [%s] >> " "$default_spool_dir"
        SPOOLING_DIR=`Enter $SPOOLING_DIR`
     else
        SPOOLING_SERVER=none
        if [ -z "$1" ]; then
           SPOOLING_DIR="`dirname $QMDIR`/spooldb"
        fi
        $INFOTEXT -n "\nPlease enter the database directory now, even if you want to spool locally,\n" \
                     "it is necessary to enter this database directory. \n\nDefault: [%s] >> " "$SPOOLING_DIR"
        SPOOLING_DIR=`Enter "$SPOOLING_DIR"`
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
      if [ "$BERKELEY" = "install" ]; then
         if [ "$IGNORE_FQDN_DEFAULT" != "true" -a "$IGNORE_FQDN_DEFAULT" != "false" ]; then
            SelectHostNameResolving
         fi      
         ignore_fqdn=$IGNORE_FQDN_DEFAULT
         default_domain=$CFG_DEFAULT_DOMAIN
         if [ "$ignore_fqdn" = "true" ]; then
            tmp_host=`echo $HOST | cut -d. -f1 | tr "[A-Z]" "[a-z]"`
            tmp_spooling=`echo $SPOOLING_SERVER | cut -d. -f1 | tr "[A-Z]" "[a-z]"`
            HOST=$tmp_host
            SPOOLING_SERVER=$tmp_spooling
         else
	         tmp_host=`echo $HOST | tr "[A-Z]" "[a-z]"`
            tmp_spooling=`echo $SPOOLING_SERVER | tr "[A-Z]" "[a-z]"`
            if [ "$default_domain" != "none" ]; then
               hasdot=`echo $tmp_host|grep '\.'`
               if [ "$hasdot" = "" ]; then
                  HOST=$tmp_host.$default_domain
               else
                  HOST=$tmp_host
               fi

               hasdot=`echo $tmp_spooling|grep '\.'`
               if [ "$hasdot" = "" ]; then
                  SPOOLING_SERVER=$tmp_spooling.$default_domain
               else
                  SPOOLING_SERVER=$tmp_spooling
               fi
            else
               HOST=$tmp_host
               SPOOLING_SERVER=$tmp_spooling
            fi
         fi

         if [ "$SPOOLING_SERVER" = "$HOST" ]; then
            # TODO: we should check if the hostname can be resolved
            # create a script to start the rpc server
            Makedir $SPOOLING_DIR

            # Deactivated the copy of DB_CONFIG file. The DB_CONFIG file is still distributed
            #DB_CONFIG_COPY="cp ./util/install_modules/DB_CONFIG $SPOOLING_DIR/DB_CONFIG"
            #ExecuteAsAdmin $DB_CONFIG_COPY
            CreateRPCServerScript
            $INFOTEXT -wait -auto $AUTO -n "Please remember these values, during Qmaster installation\n you will be asked for! Hit <RETURN> to continue!"
         else
            $INFOTEXT "Please start the Berkeley DB RPC Server installation locally on host %s!" $SPOOLING_SERVER
            $INFOTEXT -log "Please start the Berkeley DB RPC Server installation locally on host %s!" $SPOOLING_SERVER
            MoveLog
            exit 1
         fi
      fi
   return 1
   fi
}

PrepareRPCServerStart()
{
            $INFOTEXT -log "Starting rpc server on host %s!" $SPOOLING_SERVER
            $INFOTEXT "Starting rpc server on host %s!" $SPOOLING_SERVER
            ExecuteRPCServerScript start
            sleep 5
            $INFOTEXT "The Berkeley DB has been started with these parameters:\n\n"
            $INFOTEXT "Spooling Server Name: %s" $SPOOLING_SERVER
            $INFOTEXT "DB Spooling Directory: %s\n" $SPOOLING_DIR
            $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue!"

            $INFOTEXT "The Berkeley DB installation is completed now!"
            $INFOTEXT -log "The Berkeley DB installation is completed now!"
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
# The Script is either executed on the local host and not on a remote host
ExecuteRPCServerScript()
{
   if [ "$SGE_ENABLE_SMF" = "true" ]; then
      if [ "$1" = "stop" ]; then
         action=disable
      else
         action=enable
      fi
      $SVCADM $action -s "svc:/application/sge/bdb:$SGE_CLUSTER_NAME"
      if [ $? -ne 0 ]; then
         $INFOTEXT "\nFailed to %s Berkeley DB RPC Server over SMF. Check service by issuing "\
                   "svcs -l svc:/application/sge/bdb:%s" $1 $SGE_CLUSTER_NAME
         $INFOTEXT -log "\nFailed to %s Berkeley DB RPC Server over SMF. Check service by issuing "\
                        "svcs -l svc:/application/sge/bdb:%s" $1 $SGE_CLUSTER_NAME
         if [ $AUTO = true ]; then
            MoveLog
         fi
         exit 1
      fi
   else
      ExecuteAsAdmin $SGE_ROOT/$SGE_CELL/common/sgebdb $1
   fi
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


 cat $TMP_STARTUP_SCRIPT | sed -e s?\"$TMP_BDBHOME\"?\"$BDBHOMES\"?g > $TMP_STARTUP_SCRIPT.0
 `cp $TMP_STARTUP_SCRIPT.0 $TMP_STARTUP_SCRIPT`
 rm $TMP_STARTUP_SCRIPT.0


}

