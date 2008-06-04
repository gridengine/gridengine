#!/bin/sh
#
# SGE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: load_cc.sh
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

#TODO Documentation
#TODO Script man page

#TODO Main cluster copy upgrade script with user guide
#TODO Main Upgrade script with user guide

#TODO infotext?, Need to be run as Admin user
INFOTEXT=echo

if [ -z "$SGE_ROOT" -o -z "$SGE_CELL" ]; then
   $INFOTEXT "Set your SGE_ROOT, SGE_CELL first!"
   exit 1
fi

ARCH=`$SGE_ROOT/util/arch`

CAT=cat
MKDIR=mkdir
LS=ls
QCONF=$SGE_ROOT/bin/$ARCH/qconf


#Backup file, if not already done
BckFile()
{
   bckFile="${1:?Need the file name}"
   always="${2:-0}" 

   if [ ${always} = 1 -a -f ${bckFile}.sav ] ; then
      rm ${bckFile}.sav
      ret=$?
      if [ $ret != 0 ]; then
         $INFOTEXT "[CRITICAL] No permission to remove ${bckFile}.sav"
         exit 1
      fi
   fi

   if [ -f ${bckFile} -a ! -f ${bckFile}.sav ] ; then
      cp $bckFile ${bckFile}.sav
      ret=$?
      if [ $ret != 0 ]; then
         $INFOTEXT "[CRITICAL] No permission to copy ${bckFile} to ${bckFile}.sav"
         exit 1
      fi
   fi
}


#All logging is done by this functions
LogIt()
{
   urgency="${1:?Urgency is required [I,W,C]}"
   message="${2:?Message is required}"

   #log file contains all messages
   echo "${urgency} $message" >> $MESSAGE_FILE_NAME
   #log when urgency and level is meet   
   case "${urgency}${LOGGER_LEVEL}" in
      CC|CW|CI)
         $INFOTEXT "[CRITICAL] $message"
      ;;
      WW|WI)
         $INFOTEXT "[WARNING] $message"
      ;;
      II)
         $INFOTEXT "[INFO] $message"
		;;  
   esac
}


#Remove line with maching expression
RemoveLineWithMatch()
{
  remFile="${1:?Need the file name to operate}"
  remExpr="${2:?Need an expression, where to remove lines}"
  BckFile "$remFile"

  sed -e "/${remExpr}/d" $remFile > ${remFile}.tmp
  mv ${remFile}.tmp  ${remFile}
}


ReplaceLineWithMatch()
{
  repFile="${1:?Need the file name to operate}"
  repExpr="${2:?Need an expression, where to replace}" 
  replace="${3:?Need the replacement text}" 


  #awk " /${repExpr}/ { print $replace ;next} {print}" $repFile > ${repFile}.tmp
  #Return if no match
  grep ${repExpr} $repFile 2>/dev/null 2>&1
  if [ $? -ne 0 ]; then
     return
  fi
  #We need to change the file
  BckFile "$repFile"
  sed -e "s/${repExpr}/${replace}/g" $repFile > ${repFile}.tmp
  mv -f ${repFile}.tmp  ${repFile}
}


GetQuotedString()
{
  getFile="${1:?Need the file name to fetch the string from}"
  sed -e "/${remExpr}/d" $remFile > ${remFile}.tmp
  mv ${remFile}.tmp  ${remFile}
}

#Remove old IJS and add new default valuesfor the new IJS
UpdateIJSConfiguration()
{
   modFile=$1
   #Add new default to the global configuration
   if [ `echo $modFile | awk -F"/" '{ print $NF }'` = "global" ]; then
      #GLOBAL
      ReplaceLineWithMatch ${modFile} 'qlogin_command.*' "qlogin_command               builtin"
      ReplaceLineWithMatch ${modFile} 'qlogin_daemon.*'  "qlogin_daemon                builtin"
		
      ReplaceLineWithMatch ${modFile} 'rlogin_command.*' "rlogin_command               builtin"
      ReplaceLineWithMatch ${modFile} 'rlogin_daemon.*'  "rlogin_daemon                builtin"
		
      ReplaceLineWithMatch ${modFile} 'rsh_command.*'    "rsh_command                  builtin"
      ReplaceLineWithMatch ${modFile} 'rsh_daemon.*'     "rsh_daemon                   builtin"
   else
      #LOCAL
      RemoveLineWithMatch ${modFile} 'qlogin_command.*'
      RemoveLineWithMatch ${modFile} 'qlogin_daemon.*'
		
      RemoveLineWithMatch ${modFile} 'rlogin_command.*'
      RemoveLineWithMatch ${modFile} 'rlogin_daemon.*'
		
      RemoveLineWithMatch ${modFile} 'rsh_command.*'
      RemoveLineWithMatch ${modFile} 'rsh_daemon.*'
   fi
}

#Modify before load
ModifyLoad()
{
   modOpt="${1:?An option is required}"
   modFile="${2:?The file name is required}"
   #echo "ModifyLoad opt:$modOpt file:$modFile"

   #test only, comment in production
   case "$modOpt" in
      -Ae)
         #FlatFile ${modFile} 
         RemoveLineWithMatch ${modFile} 'load_values.*'
         RemoveLineWithMatch ${modFile} 'processors.*'
      ;;
      -Aconf)
	      if [ "$newIJS" = "true" ]; then
	         LogIt "I" "Updating configuration $loadFile for the new IJS"
	         UpdateIJSConfiguration $loadFile
	      fi
         ;;
   esac
   
   return $ret
}


#Resolve a result during Loading
ResolveResult()
{
   resOpt="${1:?Need an option to decide}"
   resFile="${2:?Need the file name to load}"
   resMsg="${3-""}"
   resRet="${4:?Need a return code to show the last result}"
   LogIt "I" "ResolveResult ret:$resRet,  opt:$resOpt, file:$resFile, msg:${resMsg}"

   obj=`echo ${resMsg} | awk -F'"' '{ print $2 }'` 
   obj=${obj:-unknown}

   #we are expecting troubles, possitive match required
   ret=1
   case "$resOpt" in
      -ah|-ac|-as|-am|-ao)
         #We can ignore  (already exists)
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, accepted"
               return 0
            ;;     
         esac
		;;
      -Ahgrp)
         #The proccessors and load values must be frees
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mhgrp"
               LoadConfigFile "$resFile" "-Mhgrp"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Ae)
         #The proccessors and load values must be frees
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Me"
               LoadConfigFile "$resFile" "-Me"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Acal)
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mcal"
               LoadConfigFile "$resFile" "-Mcal"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Auser)
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Muser"
               LoadConfigFile "$resFile" "-Muser"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Au)
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mu"
               LoadConfigFile "$resFile" "-Mu"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Aprj)
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mprj"
               LoadConfigFile "$resFile" "-Mprj"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Ap)
         case "$resMsg" in
            *'"accounting_summary" is missing'*)
               echo 'accounting_summary   FALSE' >> "$resFile"
               LogIt "I" "accounting_summary added, trying again"
               LoadConfigFile "$resFile" "-Ap"
               ret=$?
               return $ret
            ;;
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mp"
               LoadConfigFile "$resFile" "-Mp"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Arqs)
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mrqs"
               LoadConfigFile "$resFile" "-Mrqs"
               ret=$?
               return $ret
            ;;
            *'added'*)
               LogIt "I" "$obj added message accepted"
               #rqs shows added always, need to be eliminated 
               return 0 
            ;;
            'error: invalid option argument "-Arqs"'*)
               LogIt "I" "skipping the -Arqs option"
               return 0
            ;;
         esac
      ;;
      -Mrqs)
         case "$resMsg" in
            *'added'*)
               LogIt "I" "$obj added message accepted"
               #rqs shows added always, need to be eliminated 
               return 0 
            ;;
         esac
       ;;  
      -Ackpt)  
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mckpt"
               LoadConfigFile "$resFile" "-Mckpt"
               ret=$?
               return $ret
            ;;
         esac
      ;;
      -Aq)  
         case "$resMsg" in
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mq"
               LoadConfigFile "$resFile" "-Mq"
               ret=$?
               return $ret
            ;;
         esac
      ;;
   	#If -Aconf fails we try modify
      -Aconf)
         case "$resMsg" in
            *'value == NULL for attribute'*)
               unknown=`echo ${resMsg} | awk -F'"' '{ print $2 }'`
               ReplaceLineWithMatch ${resFile} "${unknown}*" "#${unknown}"
               LogIt "I" "$obj commented, trying to again"
               LoadConfigFile "$resFile" "$resOpt"
               ret=$?
               return $ret
            ;;
            'denied: the path given for'*)
               #FlatFile ${resFile}  
               ReplaceLineWithMatch "$resFile" 'qlogin_daemon.*' 'qlogin_daemon   /usr/sbin/in.telnetd'
               ReplaceLineWithMatch "$resFile" 'rlogin_daemon.*' 'rlogin_daemon   /usr/sbin/in.rlogind'
               LogIt "I" "wrong path corrected, trying again"
               LoadConfigFile "$resFile" "$resOpt"
               ret=$?
               return $ret               
            ;;   
            *'already exists')
               LogIt "I" "$obj already exists, trying to modify -Mconf"
               LoadConfigFile "$resFile" "-Mconf"
               ret=$?
               return $ret
            ;;
            *'will not be effective before sge_execd restart'*)
               #regular upgrade message 
               LogIt "I" "message accepted"
               return 0
            ;;
         esac
      ;;
      -Mc)  
         case "$resMsg" in
            '')
               LogIt "I" "empty output from -Mc option accepted"
               return 0
            ;;
         esac
      ;;
   esac

   case "$resMsg" in
      *'unknown attribute name'*)
         #this is a donwngrade option
         #FlatFile ${resFile} 
         RemoveLineWithMatch ${resFile} ${obj}
         LogIt "I" "$obj attribute was removed, trying again"
         LoadConfigFile "$resFile" "$resOpt"
         ret=$?
         return $ret
      ;; 
      *'added'*)
         LogIt "I" "added $obj accepted"
         addedConf=1
         return 0
      ;;
      *'modified'*)                 
         LogIt "I" "modified $obj accepted"
         return 0
      ;;
      *'changed'*)                 
         LogIt "I" "changed $obj accepted"
         return 0
      ;;
      *'does not exist') 
         #some object doesnot exists, must be reloaded
         LogIt "W" "$obj object does not exist. [REPEAT REQUIRED]"
         repeat=1
         return 1
      ;;
   esac  
   return $ret
}

#Import item to file
LoadConfigFile()
{
   loadFile="${1:?Need the file name}"
   loadOpt="${2:?Need an option}"
	
   #do not load empty files
   if [ -f "$loadFile" -a ! -s "$loadFile" ]; then
      LogIt "W" "File $loadFile is empty. Skipping ..."
      return 0
   fi

   if [ "${configLevel:=1}" -gt 20 ]; then
   	LogIt "C" "Too deep in Load Config File"
	exit 1
   fi
	
   BckFile "${loadFile}"

   configLevel=`expr ${configLevel} + 1`
   

   ModifyLoad "$loadOpt" "$loadFile" 
   loadMsg=`$QCONF $loadOpt $loadFile 2>&1`
   
   ResolveResult "$loadOpt" "$loadFile" "$loadMsg" "$ret"
   ret=$?

   if [ "$ret" != "0" ]; then 
      errorMsg="Load operation failed: qconf $loadOpt $loadFile -> $loadMsg"
      LogIt "W" "$errorMsg"
   fi

   configLevel=`expr ${configLevel} - 1`
   return $ret
}


#Import list of objects or directory of the objects
LoadListFromLocation()
{
   loadLoc="${1:?Need the location}"
   qconfOpt="${2:?Need an option}"

   LogIt "I" "qconf $qconfOpt $loadLoc"
   
   #File list
   if [ -f "$loadLoc" ]; then
      list=`$CAT $loadLoc`
      if [ -z "$list" ]; then
	 return
      fi

      for item in $list; do
         LoadConfigFile $item $qconfOpt	
      done
   #Directory list is not empty and without .sav files	
   elif [ -d "$loadLoc" ]; then
      llList=`ls -1 ${loadLoc} | grep -v '\.sav'`
      if [ -z "$llList" ]; then
         return
      fi
      #we prefer full file names 
      llList=`ls ${loadLoc}/* | grep -v '\.sav'`

      for item in $llList; do
         BckFile $item
         LoadConfigFile $item $qconfOpt
      done
   else
      #Not a file or directory (skip)
      errorMsg = "wrong directory or file: $loadLoc"
      LogIt "W" "$errorMsg"
   fi

   return $ret
}


#All SGE objects
LoadConfigurations()
{
   dir=${1:?}
   # There are the add,Load oprtions
   #     -Aattr obj_spec fname obj_instance,...   <add to object attributes>
   #     -aattr obj_spec attr_name val obj_instance,...
   #     -astnode node_path=shares,... <add share tree node>

   # -ah hostname,... <add administrative host>
   LoadListFromLocation "$dir/admin_hosts" "-ah"

   # -as hostname,... <add submit hosts>
   LoadListFromLocation "$dir/submit_hosts" "-as"

   # -am user,... <add managers>
   LoadListFromLocation "$dir/managers" "-am"

   # -ao user,... <add operators>
   LoadListFromLocation "$dir/operators" "-ao"

   # -Ae fname    <add execution host>
   LoadListFromLocation "$dir/execution" "-Ae"

   # -Mc fname <modify complex>
   LoadConfigFile "$dir/centry" "-Mc"

   # -Acal fname <add calendar>
   LoadListFromLocation "$dir/calendars" "-Acal"

   # -Ackpt fname <add ckpt. environment>
   LoadListFromLocation "$dir/ckpt" "-Ackpt"

   # -Ahgrp file <add host group config>
   LoadListFromLocation "$dir/hostgroups" "-Ahgrp"

   # -Auser fname <add user>
   LoadListFromLocation "$dir/users" "-Auser"

   # -Au fname   <add an ACL>
   LoadListFromLocation "$dir/usersets" "-Au"

   # -Aprj fname <add new project>
   LoadListFromLocation "$dir/projects" "-Aprj"

   # -Ap fname <add PE configuration>
   LoadListFromLocation "$dir/pe" "-Ap"

   # -Aq fname  <add new queue>
   LoadListFromLocation "$dir/cqueues" "-Aq"

   # -Arqs fname <add RQS configuration>
   LoadListFromLocation "$dir/resource_quotas" "-Arqs"

   # -Aconf file_list  <add configurations>
   if [ "$loadConfig" = true ]; then
      LoadListFromLocation "$dir/configurations" "-Aconf"
   fi

   # -Astree fname  <add share tree>
   LoadConfigFile "$dir/sharetree" "-Astree"

   # -Msconf  fname  <modify  scheduler   configuration
   LoadConfigFile "$dir/schedconf" "-Msconf"
}


#Load one all the configurations
LoadOnce()
{
   dir=${1:?}

   #clean added new configuration
   addedConf=0
   #clean the error code
   errorMsg=''

   LoadConfigurations "$dir"      

   # no added configuration, stop to repeat
   if [ $addedConf = 0 ]; then
      repeat=0
   fi   
}


#Reload the configuration till there is nothing to add
IterativeLoad()
{
   dir=${1:?}
   repeat=0
   loadLevel=1
   errorMsg=''
   LoadOnce "$dir"
   while [ $repeat -eq  1 ]; do
      loadLevel=`expr ${loadLevel} + 1`
      if [ "${loadLevel}" -gt 10 ]; then
         LogIt "C" "Too deep in Load Level"
         exit 1   
      fi 
      LogIt "W" "[REPEAT LOAD]"
      LoadOnce "$dir"
   done    

   if [ -n "$errorMsg" ]; then 
      LogIt "C" "$errorMsg"
      exit 1
   fi
}

########
# MAIN #
########
DIR="${1:?The load directory is required}"
LOGGER_LEVEL=${2:-'W'}

newIJS=false
loadConfig=true

shift
shift
ARGC=$#
while [ $ARGC -gt 0 ]; do
   case $1 in
      -newijs)
         newIJS=true
         LogIt "I" "LOAD invoked with -newijs"
         ;;
      -noconfig)
         LogIt "I" "LOAD invoked with -noconfig"
         loadConfig=false
         ;;
      *)
         echo "Invalid argument \'$1\'"
         ;;
   esac
   shift
   ARGC=`expr $ARGC - 1`
done

#TODO: Test if supported qmaster host + admin host

MESSAGE_FILE_NAME="${DIR}/load.log"
BckFile "${MESSAGE_FILE_NAME}" "1"


CURRENT_VERSION=`$QCONF -help | sed  -n '1,1 p'` 2>&1
ret=$?
if [ "$ret" != "0" ]; then
   $INFOTEXT "ERROR: qconf -help failed"
   LogIt "C" "qmaster is not installed"
   exit 1
fi
$QCONF -sq > /dev/null 2>&1
ret=$?
if [ "$ret" != "0" ]; then
   $INFOTEXT "ERROR: qconf -sq failed. Qmaster is not running?"
   LogIt "C" "qmaster is not running"
   exit 1
fi

LOAD_VERSION=`cat $DIR/version`
echo "LOAD $DIR" > $MESSAGE_FILE_NAME
echo "$CURRENT_VERSION" >> $MESSAGE_FILE_NAME
echo "$LOAD_VERSION" >> $MESSAGE_FILE_NAME

#Clear .sav file if present
#TODO: Remove BckFile and .sav files with /tmp/timestamp dir
#TODO: too long error might occur?
for f in `find $DIR -type f -name "*.sav"` ; do
   mv $f `echo "$f" | awk '{ print substr($0,1,length($0) - 4);}'`
   if [ $? -ne 0 ]; then
      LogIt "C" "Found .sav file in the backup directory. Appempt to rename them to the original names failed"
      exit 1
   fi
done


echo "Loading saved cluster configuration from $DIR ..."

IterativeLoad "$DIR"

echo "LOADING FINISHED" >> $MESSAGE_FILE_NAME
echo "Done"
exit 0
