#! /bin/sh
#
# SGE/SGEEE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: sge_config_berkeley.sh
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
   $INFOTEXT -u "\nBerkeley Database spooling parameters"
   $INFOTEXT "\nWe can use local spooling, which will go to a local filesystem" \
             "\nor use a RPC Client/Server mechanism. In this case, qmaster will" \
             "\ncontact a RPC server running on a separate server machine." \
             "\nIf you want to use the SGE shadowd, you have to use the " \
             "\nRPC Client/Server mechanism.\n"
   $INFOTEXT "\nEnter database server (none for local spooling)\n" \
             "or hit <RETURN> to use default [%s] >> " $SPOOLING_SERVER
             SPOOLING_SERVER=`Enter $SPOOLING_SERVER`

   $INFOTEXT "\nEnter the database directory\n" \
             "or hit <RETURN> to use default [%s] >> " $SPOOLING_DIR
             SPOOLING_DIR=`Enter $SPOOLING_DIR`
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
      echo $SPOOLING_DIR >> $SPOOLING_DIR/bdbhomes
      CreateRPCServerScript
      $INFOTEXT "Now we have to startup the rc script >%s< on the RPC server machine\n" $SPOOLING_DIR/sgebdb
      $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "Shall the installation script try to start the RPC server? (y/n) [y] >>"

      if [ $? = 0 -a $AUTO = "false" ]; then
         $INFOTEXT -log "Starting rpc server on host %s!" $SPOOLING_SERVER
         $INFOTEXT "Starting rpc server on host %s!" $SPOOLING_SERVER
         exec $SPOOLING_DIR/sgebdb start &
         sleep 5
      else
         $INFOTEXT "Please start the rc script >%s< on the RPC server machine\n" $SPOOLING_DIR/sgebdb
         $INFOTEXT -auto $AUTO "Hit <Enter> to continue! >>"
      fi

   return 1
   fi
}

CreateRPCServerScript()
{
   RPCSCRIPT=$SPOOLING_DIR/sgebdb
   Execute sed -e "s%GENROOT%${SGE_ROOT_VAL}%g" \
               -e "s%GENCELL%${SGE_CELL_VAL}%g" \
               -e "s%SPOOLING_DIR%${SPOOLING_DIR}%g" \
               -e "/#+-#+-#+-#-/,/#-#-#-#-#-#/d" \
               util/rpc_startup_template > ${RPCSCRIPT}
   Execute $CHMOD a+x $RPCSCRIPT

}


CheckLocalFilesystem()
{  
   FS=`dirname $1`
   case $ARCH in
      solaris*)
         df -l $FS >/dev/null 2>&1
         if [ $? -eq 0 ]; then
            return 1
         else
            return 0
         fi
         ;;
      *linux)
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



