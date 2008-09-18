#! /bin/sh 
#
# SGE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_qmaster.sh
# Module: qmaster installation functions
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
# The service tags support
# uses $UPDATE, $QMASTER, 
#
ServiceTagsSupport()
{
   SGE_ST_CMD=util/sgeST/sge_st

   if [ "$QMASTER" = "install" ]; then

      st_supported=`$SGE_ST_CMD "supported"`
      if [ "$st_supported" = "true" ]; then
         $CLEAR
         $INFOTEXT "Service Tags are a monitoring technology by SUN Microsystems.\n" \
                   "A Service Tag enables automatic discovery of systems, software, and services\n" \
                   "and allows the asset information to be monitored over a local network.\n"

         $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "\nAre you going to enable Service Tags support? (y/n) [y] >> "
         ret=$?
         if [ "$AUTO" != "true" ]; then
            if [ $ret = 0 ]; then
               SERVICE_TAGS="enable"
            else
               SERVICE_TAGS="disable"
            fi
         fi

         if [ "$SERVICE_TAGS" = "enable" ]; then
            $SGE_ST_CMD "enable" > /dev/null
         else
            $SGE_ST_CMD "disable" > /dev/null
         fi
      fi
   elif [ $QMASTER = "uninstall" ]; then
      $SGE_ST_CMD "unregister" > /dev/null 
   fi
}
