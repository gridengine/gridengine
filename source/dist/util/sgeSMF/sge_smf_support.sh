#!/bin/sh
#
# Sun Grid Engine SMF support script
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

#TODO:
#1 - Do not match case QMaster = qmaster
#2 - Need to get the host where we will install the service. Problem root on remote host?

#---------------------------------------------------------------------------
# This script requires the that $SGE_ROOT and $SGE_CELL be set
#

SGE_SMF_VERSION="v62"      #Will become part of the service name

#---------------------------------------------------------------------------
# Show the usage of the standalone command
SMFusage()
{
   $ECHO "Usage: sge_smf <command>"
   $ECHO ""
   $ECHO "Commands:"
   $ECHO "   register   [qmaster|shadowd|execd|bdb|dbwriter|all] CLUSTER_NAME   ... register SGE as SMF service"
   $ECHO "   unregister [qmaster|shadowd|execd|bdb|dbwriter|all] CLUSTER_NAME   ... unregister SGE SMF service"
   #$ECHO "   enabled         ... check if the SGE via SMF is enabled"
   $ECHO "   supported       ... check if the SMF can be used on current host"
   $ECHO "   help            ... this help"
   $ECHO
   $ECHO "CLUSTER_NAME ... name of this cluster (installation), used as instance name in SMF"
   $ECHO "                 MUST be the same as you entered during the istallation"
}


#-------------------------------------------------------------------------
# Arch_variables Setting up common variables and paths
#
ArchVariables()
{
   if [ -f "$SGE_ROOT/util/arch_variables" ]; then
      . $SGE_ROOT/util/arch_variables
   elif [ -f "./util/arch_variables" ]; then
      . ./util/arch_variables
   else
      $ECHO "Could not source arch_variables"
      exit 1
   fi
}


SED="/usr/bin/sed"
SVCADM="/usr/sbin/svcadm"
SVCCFG="/usr/sbin/svccfg"

#Architecture related variables echo
ArchVariables

if [ -z "$SGE_ROOT" -o -z "$SGE_CELL" ]; then
   $ECHO "\$SGE_ROOT and \$SGE_CELL must be set!"
   exit 1
fi

if [ -z "$ARCH" ]; then
   ARCH=`$SGE_ROOT/util/arch`
fi

if [ -f /lib/svc/share/smf_include.sh ]; then 
   . /lib/svc/share/smf_include.sh
   smf_present
   hasSMF="$?"
   if [ $hasSMF -ne 0 ]; then
      hasSMF=0
   else
      hasSMF=1
   fi
else
   hasSMF=0
fi


#-------------------------------------------------------------------------
# SMFImportService: Import service descriptor
#
SMFImportService() 
{
   file="$1"
   SMFValidateContent $file
   res="$?"
   if [ "$res" != 0 ]; then
      $ECHO "Service descriptor $file is corrupted, exiting!"
      exit 1
   fi
   #Strangly svccfg import does not return non-zero exit code if there was 
   #an error such as permission denied
   res=`$SVCCFG import $file 2>&1 | head -1`
   if [ -n "$res" ]; then
      $ECHO $res
      $ECHO "Importing service was NOT successful!"
      exit 1
   fi
}

#-------------------------------------------------------------------------
# SMFValidateContent: Validate service descriptor
#
SMFValidateContent()
{
   return `$SVCCFG validate "$1"`
}

#-------------------------------------------------------------------------
# SMFReplaceXMLTemplateValues: replace variables
#
SMFReplaceXMLTemplateValues()
{
   file="$1"
   cat $file > $file.tmp
   #Replace SGE_ROOT
   $SED -e "s|@@@CLUSTER_NAME@@@|$SGE_CLUSTER_NAME|g" \
        -e "s|@@@SGE_SMF_VERSION@@@|$SGE_SMF_VERSION|g" \
        -e "s|@@@SGE_ROOT@@@|$SGE_ROOT|g" \
        -e "s|@@@SGE_CELL@@@|$SGE_CELL|g" \
        -e "s|@@@SGE_QMASTER_PORT@@@|$SGE_QMASTER_PORT|g" \
        -e "s|@@@SGE_EXECD_PORT@@@|$SGE_EXECD_PORT|g" $file.tmp > $file
   #Delete tmp file
   $RM $file.tmp
}

#-------------------------------------------------------------------------
# SMFCreateAndImportServiceFromTemplate: Replaces template values and 
#                                        imports it to the repository
#
SMFCreateAndImportService()
{
   prefix="$SGE_ROOT"
   template_file="$prefix/util/sgeSMF/$1_template.xml"
   suffix="${SGE_CLUSTER_NAME}"
   service_file="$prefix/util/sgeSMF/$1_$suffix.xml"
   if [ ! -f $template_file ]; then
      echo "$template_file is missing!"
      exit 1
   fi
   cat $template_file > $service_file
   SMFReplaceXMLTemplateValues $service_file
   SMFImportService $service_file
   $RM $service_file
   $ECHO "$1 service imported successfully!"
}

#---------------------------------------------------------------------------
# Register SMF service
# return Registered messages or nothing
SMFRegister()
{   
   case "$1" in
   qmaster)
      SMFCreateAndImportService "qmaster"
      ;;
   shadowd)
      SMFCreateAndImportService "shadowd"
      ;;
   execd)
      SMFCreateAndImportService "execd"
      ;;
   bdb)
      SMFCreateAndImportService "bdb"
      ;;
   berkeleydb)
      SMFCreateAndImportService "bdb"
      ;;
   dbwriter)
      SMFCreateAndImportService "dbwriter"
      ;;
   all)
      SMFCreateAndImportService "qmaster"
      SMFCreateAndImportService "shadowd"
      SMFCreateAndImportService "execd"
      ;;
   *)
      $ECHO "Unknown parameter to sge_smf register"
      exit 1
      ;;
   esac
   return 0
}


#-------------------------------------------------------------------------
# SMFHaltAndDeleteService: Stops the service and deletes it
# arg1 ... SMF service name
SMFHaltAndDeleteService()
{
   if [ ! -f "$SGE_ROOT/$SGE_CELL/common/cluster_name" ]; then
      $ECHO "Missing $SGE_ROOT/$SGE_CELL/common/cluster_name file!"
      exit 1
   fi
   if [ -z "$SGE_SMF_VERSION" ]; then
      $ECHO "Missing SGE_SMF_VERSION!!!"
      exit 1
   fi
   cluster_name=`cat "$SGE_ROOT/$SGE_CELL/common/cluster_name"`
   service_name="svc:/application/sge_$SGE_SMF_VERSION/$1:$cluster_name"

   $INFOTEXT "$1 is going down ...., please wait!"
   $SVCADM disable -s "$service_name"
   if [ "$?" -ne 0 ]; then
      "Could not disable the service \"$service_name\"! Exiting"
      exit 1
   fi
   $INFOTEXT "$1 is down!"
   $SVCCFG delete "$service_name"
   if [ "$?" -ne 0 ]; then
      "Could not delete the service \"$service_name\"! Exiting"
      exit 1
   fi
   $ECHO "Unregistered $service_name"
}

#---------------------------------------------------------------------------
# Unregister SMF service
# return registered or not to stdout
SMFUnregister()
{   
   case "$1" in
   qmaster)
      SMFHaltAndDeleteService "qmaster"
      ;;
   shadowd)
      SMFHaltAndDeleteService "shadowd"
      ;;
   execd)
      SMFHaltAndDeleteService "execd"
      ;;
   bdb)
      SMFHaltAndDeleteService "bdb"
      ;;
   berkeleydb)
      SMFHaltAndDeleteService "bdb"
      ;;
   dbwriter)
      SMFHaltAndDeleteService "dbwriter"
      ;;
   all)
      SMFHaltAndDeleteService "qmaster"
      SMFHaltAndDeleteService "shadowd"
      SMFHaltAndDeleteService "execd"
      ;;
   *)
      $ECHO "Unknown parameter to sge_smf unregister"
      exit 1
      ;;
   esac
   return 0
}


#---------------------------------------------------------------------------
# Main routine
# PARAMS: $command 
# return a stdout output
SMF()
{   
   cmd="$1"

   if [ "$#" -lt 1 -o  "$#" -gt 3 -o "$cmd" = "-h" -o "$cmd" = "help"  -o "$cmd" = "--help" ]; then
      SMFusage
      exit 1
   fi
 
   if [ "$cmd" = "register" -o "$cmd" = "unregister" ]; then
      if [ "$#" -lt 2 ]; then
         echo "Missing an argument!"
         SMFusage
         exit 1
      fi
      opt="$2"
      #Check we have valid values
      if [ "$opt" != "qmaster" -a "$opt" != "execd" -a "$opt" != "all" ]; then
         $ECHO "Invalid argument $opt!"
         SMFusage
         exit 1
      fi
      #We source the settings if we don't have qmaster or execd port 
      if [ -z "$SGE_QMASTER_PORT" -o -z "$SGE_EXECD_PORT" ]; then
         . "$SGE_ROOT/$SGE_CELL/common/settings.sh"
      fi
      SGE_CLUSTER_NAME="$3"
      if [ -z "$SGE_CLUSTER_NAME" ]; then
         $ECHO "Missing cluster name."
         SMFusage
         exit 1
      fi
   fi

   #Standalone version of supported. We never use this option in installation script
   if [ "$cmd" = "supported" ]; then
      if [ $hasSMF ]; then
         $ECHO "SMF is supported on your system."
      fi
      exit 0
   fi
   #We return if we don't have SMF
   if [ ! $hasSMF ]; then
      exit 1
   fi

   if [ "$cmd" = "register" ]; then
      SMFRegister $opt
   elif [ "$cmd" = "unregister" ]; then
      SMFUnregister $opt
   else
      SMFusage
      exit 1
   fi
   return 0
}
