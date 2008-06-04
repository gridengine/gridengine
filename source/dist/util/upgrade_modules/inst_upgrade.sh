#!/bin/sh
#
# SGE configuration script (Upgrade/Downgrade)
# Scriptname: inst_upgrade.sh
# Module: common upgrade functions
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


#--------------------------------------------------------------------------
# WelcomeTheUserUpgrade - upgrade welcome message
#
WelcomeTheUserUpgrade()
{
   $INFOTEXT -u "\nWelcome to the Grid Engine Upgrade"
   $INFOTEXT "\nBefore you continue with the upgrade please read these hints:\n\n" \
             "   - Your terminal window should have a size of at least\n" \
             "     80x24 characters\n\n" \
             "   - The INTR character is often bound to the key Ctrl-C.\n" \
             "     The term >Ctrl-C< is used during the upgrade if you\n" \
             "     have the possibility to abort the upgrade\n\n" \
             "The upgrade procedure will take approximately 1-2 minutes.\n"
   $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
   $CLEAR
}

#-------------------------------------------------------------------------------
# GetBackupedAdminUser: Get admin user form the cluster configuration backup
# TODO: Cleanup duplicit with inst_common.sh GetAdminUser()
GetBackupedAdminUser()
{
   ADMIN_USER=`BootstrapGetValue "$UPGRADE_BACKUP_DIR/cell" admin_user`
   euid=`$SGE_UTILBIN/uidgid -euid`

   TMP_USER=`echo "$ADMINUSER" |tr "A-Z" "a-z"`
   if [ \( -z "$TMP_USER" -o "$TMP_USER" = "none" \) -a $euid = 0 ]; then
      ADMINUSER=default
   fi

   if [ "$SGE_ARCH" = "win32-x86" ]; then
      HOSTNAME=`hostname | tr "a-z" "A-Z"`
      ADMINUSER="$HOSTNAME+$ADMINUSER"
   fi
}

#TODO: Use it in inst_sge
#-------------------------------------------------------------------------------
# FileGetValue: Get values from a file for appropriate key
#  $1 - PATH to the file
#  $2 - key: e.g: qmaster_spool_dir | ignore_fqdn | default_domain, etc.
FileGetValue()
{
   if [ $# -ne 2 ]; then
      $INFOTEXT "Expecting 2 arguments for FileGetValue. Got %s." $#
      exit 1
   fi
   if [ ! -f "$1" ]; then
      $INFOTEXT "No file %s found" $1
      exit 1
   fi
   echo `cat $1 | grep "$2" | awk '{ print $2}' 2>/dev/null`
}

#Helper to get bootstrap file values
#See FileGetValue
BootstrapGetValue()
{
   FileGetValue "$1/bootstrap" $2
}

#-------------------------------------------------------------------------------
# CheckUpgradeUser: Check if valid user performs the upgrade
#
CheckUpgradeUser()
{
   if [ $euid -ne 0 -a `whoami` != "$ADMINUSER" ]; then
      $INFOTEXT "\nUpgrade procedure must be started as a root or admin user.\nCurrent user is `whoami`."
      exit 1
   elif [ "$OLD_ADMIN_USER" != "$ADMIN_USER" ]; then
      $INFOTEXT "\nCannot use this backup for the upgrade.\n" \
                "   current admin user: '$OLD_ADMIN_USER'\n" \
	        "   admin user in the backup: '$ADMIN_USER'\n" \
	        "Seems like this is not a backup of this cluster. If you want really continue \n" \
	        "with this backup manually edit bootstrap file in the backup to have the same \n" \
	        "admin user as your cluster currently has."
      exit 1
   fi
}

#-------------------------------------------------------------------------------
# GetBackupDirectory: Ask for backup directory. Udes during the upgrade.
#
GetBackupDirectory()
{
   done=false
   while [ $done = false ]; do
      $CLEAR
      $INFOTEXT -n "\nEnter path to a backup directory >> "
      eval UPGRADE_BACKUP_DIR=`Enter $UPGRADE_BACKUP_DIR`
      version=`cat "${UPGRADE_BACKUP_DIR}/version" 2>/dev/null`
      backup_date=`cat "${UPGRADE_BACKUP_DIR}/backup_date" 2>/dev/null`
      if [ -n "$version" -a -n "$backup_date" -a -d "${UPGRADE_BACKUP_DIR}/cell" ]; then
         #Ask if correct
	 $INFOTEXT -n "\nFound backup from $version version created on $backup_date"
	 $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "\nContinue with this backup directory (y/n) [y] >> "
         if [ $? -eq 0 ]; then
            done=true
	         return 0
         fi
      else
         $INFOTEXT -n "\n$UPGRADE_BACKUP_DIR is not a valid backup directory!"
	 $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "\nEnter a new backup directory or exit ('n') (y/n) [y] >> "
	 if [ $? -ne 0 ]; then
	    exit 0
	 fi
      fi
   done
}

#-------------------------------------------------------------------------------
# RestoreCell: Restore backuped cell directory
#   $1 - BACKUPED_CELL_DIRECTORY
RestoreCell()
{
   old_cell=$1
   if [ ! -d  "$SGE_ROOT/$SGE_CELL/common" ]; then
      ExecuteAsAdmin mkdir -p "$SGE_ROOT/$SGE_CELL/common"
   fi
   
   FILE_LIST="bootstrap
qtask
sge_aliases
sge_request
shadow_masters
accounting
dbwriter.conf"

   for f in $FILE_LIST; do
      if [ -f "${old_cell}/$f" ]; then
         ExecuteAsAdmin cp -f "${old_cell}/$f" "$SGE_ROOT/$SGE_CELL/common"
      fi
   done
   
   #Modify bootstrap file
   ExecuteAsAdmin $CHMOD 666 "$SGE_ROOT/$SGE_CELL/common/bootstrap"
   #Version string
   ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 666 'Version.*' "Version: $SGE_VERSION"
   #ADMIN_USER
   if [ $ADMINUSER != default ]; then
      admin_user_value="admin_user             $ADMINUSER"
   else
      admin_user_value="admin_user             none"
   fi
   ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 666 'admin_user.*' "$admin_user_value"
   #TODO: default_domain, ignore_fqdn?
   #BINARY PATH
   ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 666 'binary_path.*' "binary_path             $SGE_ROOT/bin"
   #QMASTER SPOOL DIR
   ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 666 'qmaster_spool_dir.*' "qmaster_spool_dir       $QMDIR"
   #PRODUCT MODE
   ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 666 'security_mode.*' "security_mode           $PRODUCT_MODE"
   
   #TODO: remove gdi_threads
   
   #Add threads info if missing
   cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "threads" >/dev/null 2>&1
   do_bootstrap_upgrade=$?
   if [ $do_bootstrap_upgrade -eq 1 ]; then
      $ECHO "listener_threads        2" >> $SGE_ROOT/$SGE_CELL/common/bootstrap
      $ECHO "worker_threads          2" >> $SGE_ROOT/$SGE_CELL/common/bootstrap
      $ECHO "scheduler_threads       1" >> $SGE_ROOT/$SGE_CELL/common/bootstrap
   fi
   
   cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "jvm_threads" >/dev/null 2>&1
   if [ $? -ne 0 -a "$SGE_ENABLE_JMX" = "true" ]; then
      $ECHO "jvm_threads             1" >> $SGE_ROOT/$SGE_CELL/common/bootstrap
   else
      $ECHO "jvm_threads             0" >> $SGE_ROOT/$SGE_CELL/common/bootstrap
   fi
   ExecuteAsAdmin $CHMOD 644 "$SGE_ROOT/$SGE_CELL/common/bootstrap"
}

#-------------------------------------------------------------------------------
# RestoreJMX: Ask if JMX setting from the backup should be used
#   $1 - BACKUPED_JMX_DIRECTORY
RestoreJMX()
{
   old_jmx=$1
   if [ "$SGE_ENABLE_JMX" = "true" -a -d "${old_jmx}" ]; then
      $CLEAR
      $INFOTEXT -n "\nFound JMX settings in the backup"
      $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "\nUse the JMX settings from the backup ('y') or reconfigure ('n') (y/n) [y] >> "
      if [ $? -eq 0 ]; then
	 #Use backup
	 ExecuteAsAdmin cp -r "${old_jmx}" "$SGE_ROOT/$SGE_CELL/common"
	 SGE_ENABLE_JMX=false
      fi
   fi
}

#-------------------------------------------------------------------------------
# NewIJS: Ask if JMX setting from the backup should be used
#   $1 - BACKUPED_JMX_DIRECTORY
SavedOrNewIJS()
{
   $CLEAR
   $INFOTEXT -u "Interactive Job Support (IJS) selection"
   $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "\nUse IJS from the backup ('y') or use new defaults ('n') (y/n) [y] >> "
   if [ $? -ne 0 ]; then
      $INFOTEXT -n "\nUsing new interactive job support default setting for a new installation."
      newIJS=true
   fi
   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
   $CLEAR
}

#-------------------------------------------------------------------------------
# AskForNewSequenceNumber: Ask for the next sequence number
#   $1 - file with the seq_number
#   $2 - "job" | "ar"
AskForNewSequenceNumber()
{
   NEXT_SEQ_NUMBER=""
   NEXT_SEQ_NUMBER=`cat "${1}" 2>/dev/null`
   if [ -z "$NEXT_SEQ_NUMBER" -o "$NEXT_SEQ_NUMBER" -lt 0 ]; then
      NEXT_SEQ_NUMBER=0
   else
      #Add 1000 and round up
      NEXT_SEQ_NUMBER=`expr $NEXT_SEQ_NUMBER / 1000 + 1`
      NEXT_SEQ_NUMBER=`expr $NEXT_SEQ_NUMBER \* 1000`
   fi
   if [ "$2" != "job" -a "$2" != "AR" ]; then
      $INFOTEXT "Invalid value '"$2"' provided to AskForNewSequenceNumber"
      exit 1
   fi
   $CLEAR
   $INFOTEXT -u "Select next %s number" "$2"
   $INFOTEXT -n "\nBackup contains last %s number. We added 1000 and rounded it up to 1000s.\n" \
                "Increase the number, if appropriate.\n" \
		"Select the new next %s number [%s] >> " "$2" "$2" "$NEXT_SEQ_NUMBER"
   eval NEXT_SEQ_NUMBER=`Enter $NEXT_SEQ_NUMBER`
   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
   $CLEAR
}

#-------------------------------------------------------------------------------
# RestoreSequenceNumberFiles: Ask for the next sequence number
#   $1 - qmaster_spool_dir
RestoreSequenceNumberFiles()
{
   QMASTER_SPOOL_DIR=$1
   AskForNewSequenceNumber "${UPGRADE_BACKUP_DIR}/jobseqnum" "job"
   SafelyCreateFile "$QMASTER_SPOOL_DIR/jobseqnum" 644 $NEXT_SEQ_NUMBER
   AskForNewSequenceNumber "${UPGRADE_BACKUP_DIR}/arseqnum" "AR"
   SafelyCreateFile "$QMASTER_SPOOL_DIR/arseqnum" 644 $NEXT_SEQ_NUMBER
}

#Copy of inst_execd.sh UnInstWinHelperSvc but modified for pre62
#TODO: needs to be done on every windows execd host
#TODO: cleanup duplicit with UnInstWinHelper()
UninstWinHelperPre62() {
   tmp_path=$PATH
   PATH=/usr/contrib/win32/bin:/common:$SAVED_PATH
   export PATH
	
   WIN_SVC="N1 Grid Engine Helper Service"
   WIN_DIR=`winpath2unix $SYSTEMROOT`
   $INFOTEXT " Testing, if windows helper service is installed!\n"
   eval "net pause \"$WIN_SVC\"" > /dev/null 2>&1
   ret=$?
   if [ "$ret" = 0 ]; then
      ret=2
      $INFOTEXT "   ... a service is installed!"
      $INFOTEXT -log "   ... a service is installed!"
      $INFOTEXT "   ... stopping service!"
      $INFOTEXT -log "   ... stopping service!"

      while [ "$ret" -ne 0 ]; do
         eval "net continue \"$WIN_SVC\"" > /dev/null 2>&1
         ret=$?
      done
   else
      $INFOTEXT "   ... no service installed!"   
      $INFOTEXT -log "   ... no service installed!"   
   fi
	
   if [ -f "$WIN_DIR"/N1_Helper_Service.exe ]; then
      $INFOTEXT "   ... found service binary!" 
      $INFOTEXT -log "   ... found service binary!" 
      $INFOTEXT "   ... uninstalling service!"
      $INFOTEXT -log "   ... uninstalling service!"
      $WIN_DIR/Sun_Helper_Service.exe -uninstall
      rm $WIN_DIR/Sun_Helper_Service.exe
   fi

   PATH=$tmp_path
   export PATH
}

#Select spooling method
# $1 - backued spooling method
SelectNewSpooling() 
{
   backuped_spooling_method=$1
   if [ "$backuped_spooling_method" != "berkeleydb" -a "$backuped_spooling_method" != "classic" ]; then
      $INFOTEXT "Invalid arg $1 to SelectNewSpooling"
      exit 1
   fi
	
   keep=false
	
   $CLEAR
   $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "\nUse backuped %s spooling method ('y') or use new spooling configuration ('n') (y/n) [y] >> " \
             $backuped_spooling_method
   if [ $? -eq 0 ]; then
      keep=true
      SPOOLING_METHOD=$backuped_spooling_method
      case $SPOOLING_METHOD in 
         classic)
            SetSpoolingOptionsClassic
            ;;
         berkeleydb)
	    #TODO: need new upgrade questions with defaults from the backup!
	    #      inst_berkeley - SpoolingQueryChange might optionally accept sugegstion defualt value
	    SPOOLING_DIR=`BootstrapGetValue "${UPGRADE_BACKUP_DIR}/cell" "spooling_params"`
            SetSpoolingOptionsBerkeleyDB $SPOOLING_DIR
            ;;
         esac
   else
      SetSpoolingOptions
   fi
	
   if [ "$keep" = false ]; then
      ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 644 'spooling_method.*' "spooling_method         $SPOOLING_METHOD"
      ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 644 'spooling_lib.*'    "spooling_lib            $SPOOLING_LIB"
      ReplaceLineWithMatch "$SGE_ROOT/$SGE_CELL/common/bootstrap" 644 'spooling_params.*' "spooling_params         $SPOOLING_ARGS"
   fi
}

#AddDummyConfiguration - add a dummy configuration so we can start qmaster
#
AddDummyConfiguration() 
{
   CFG_EXE_SPOOL=$QMDIR/execd
   MAILER=mailx
   XTERM=xterm
   CFG_MAIL_ADDR=none
   CFG_GID_RANGE=20000-21000
   QLOGIN_COMMAND=builtin
   QLOGIN_DAEMON=builtin
   RLOGIN_COMMAND=builtin
   RLOGIN_DAEMON=builtin
   RSH_COMMAND=builtin
   RSH_DAEMON=builtin
   #TODO: add timestamp
   TMPC=/tmp/configuration
   rm -f $TMPC
   SafelyCreateFile $TMPC 666 ""
   PrintConf >> $TMPC 
   ExecuteAsAdmin $SPOOLDEFAULTS configuration $TMPC
   ExecuteAsAdmin rm -f $TMPC
}

ReplaceLineWithMatch()
{
  repFile="${1:?Need the file name to operate}"
  filePerms="${2:?Need file final permissions}"
  repExpr="${3:?Need an expression, where to replace}" 
  replace="${4:?Need the replacement text}" 

  #Return if no match
  grep "${repExpr}" $repFile >/dev/null 2>&1

  #We need to change the file
  ExecuteAsAdmin touch ${repFile}.tmp
  ExecuteAsAdmin chmod 666 ${repFile}.tmp
  sed -e "s|${repExpr}|${replace}|g" $repFile >> ${repFile}.tmp
  ExecuteAsAdmin mv -f ${repFile}.tmp  ${repFile}
  ExecuteAsAdmin chmod ${filePerms} ${repFile}
}

AddNewConfigurations()
{
   $CLEAR
   $INFOTEXT -n "\nYou must provide a different value from the suggested in the next configuration screen!\n"
   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
   $CLEAR
   #Need to modify the configuration form the backup in case of copy upgrade
   if [ "$newIJS" != true -a "$UPGRADE_MODE" = copy ]; then
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/global" "qlogin_daemon"`
      if [ -n "$tmp_value" ]; then
         QLOGIN_DAEMON=$tmp_value
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/global" "qlogin_command"`
      if [ -n "$tmp_value" ]; then
         QLOGIN_COMMAND=$tmp_value
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/global" "rlogin_daemon"`
      if [ -n "$tmp_value" ]; then
          RLOGIN_DAEMON=$tmp_value
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/global" "rlogin_command"`
      if [ -n "$tmp_value" ]; then
         RLOGIN_COMMAND=$tmp_value
      else
         RLOGIN_COMMAND=undef
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/global" "rsh_daemon"`
      if [ -n "$tmp_value" ]; then
         RSH_DAEMON=$tmp_value
      else
         RSH_DAEMON=undef
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/global" "rsh_command"`
      if [ -n "$tmp_value" ]; then
         RSH_COMMAND=$tmp_value
      else
         RSH_COMMAND=undef
      fi
   fi
   GID_RANGE=`FileGetValue "$UPGRADE_BACKUP_DIR/configurations/global" "gid_range"`
   AddConfiguration `FileGetValue "$UPGRADE_BACKUP_DIR/configurations/global" "execd_spool_dir"` `FileGetValue "$UPGRADE_BACKUP_DIR/configurations/global" "administrator_mail"`
   
   #Create local configuration with correct IJS settings
   if [ "$newIJS" = false -a "$UPGRADE_MODE" = copy -a -f "${UPGRADE_BACKUP_DIR}/configurations/${HOST}" ]; then
      #Just modify IJS related values
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/${HOST}" "qlogin_daemon"`
      if [ -n "$tmp_value" ]; then
         QLOGIN_DAEMON=$tmp_value
      else
         QLOGIN_DAEMON=undef
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/${HOST}" "rlogin_daemon"`
      if [ -n "$tmp_value" ]; then
         RLOGIN_DAEMON=$tmp_value
      else
         RLOGIN_DAEMON=undef
      fi
      tmp_value=""
      tmp_value=`FileGetValue "${UPGRADE_BACKUP_DIR}/configurations/${HOST}" "rsh_daemon"`
      if [ -n "$tmp_value" ]; then
         RSH_DAEMON=$tmp_value
      else
         RSH_DAEMON=undef
      fi
   fi
   AddLocalConfiguration
}
