#!/bin/sh
#
# SGE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_execd_uninst.sh
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
#
# set -x

WelcomeUninstall()
{
   if [ $AUTO = true ]; then
      return
   fi

   $INFOTEXT -u "Grid Engine unistallation"
   if [ "$ALL_EXECDS" = true ]; then
      $INFOTEXT "\nYour are going to uninstall all execution hosts!\n" \
                "If you are not sure what you are doing, than please stop\n" \
                "this procedure with <CTRL-C>!\n" 
   else
      $INFOTEXT "\nYour are going to uninstall a execution host!\n" \
                "If you are not sure what you are doing, than please stop\n" \
                "this procedure with <CTRL-C>!\n"
   fi
   
   $INFOTEXT -wait -n "\nHit <ENTER>, to continue! >>"
}

FetchHostname()
{
   if [ $AUTO = "true" ]; then
      HOST=$EXEC_HOST_LIST_RM
   fi

   if [ "$ALL_EXECDS" = true ]; then
      HOST=`qconf -sel`
   fi

   for h in $HOST; do
     qconf -se $h
     ret=$?
     if [ $ret = 0 ]; then
        $INFOTEXT "Removing execution host %s now!" $h
        $INFOTEXT -log "Removing execution host %s now!" $h
        $INFOTEXT "Disabling queues now!"
        $INFOTEXT -log "Disabling queues now!"
        DisableQueue $h
        SuspendQueue $h
        SuspendJobs $h
        RescheduleJobs $h
        RemoveExecd $h
        RemoveQueues $h
        RemoveSpoolDir $h

     else
        $INFOTEXT "%s is not an execution host" $h
        $INFOTEXT -log "%s is not an execution host" $h
     fi

   done

}

DisableQueue()
{
   exechost=$1

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "Disabling queue %s now" $q
     $INFOTEXT -log "Disabling queue %s now" $q
     qmod -d $q

   done


}

SuspendQueue()
{
   exechost=$1

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "Suspending queue %s now" $q
     $INFOTEXT -log "Suspending queue %s now" $q
     qmod -sq $q

   done


}

SuspendJobs()
{
   exechost=$1

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "Suspending Checkpointing Jobs on queue %s now!" $q 
     $INFOTEXT -log "Suspending Checkpointing Jobs on queue %s now!" $q 
     qmod -sj $q

   done

}

RescheduleJobs()
{
   exechost=$1

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "Rescheduling Jobs on queue %s now!" $q 
     $INFOTEXT -log "Rescheduling Jobs on queue %s now!" $q 
     qmod -r $q

   done

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "There are still running jobs on %s!" $q
     $INFOTEXT -log "There are still running jobs on %s!" $q
     $INFOTEXT "... trying to force a reschedule!"
     $INFOTEXT -log "... trying to force a reschedule!"
     qmod -f -r $q 

   done

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "There are still running jobs on %s!" $q
     $INFOTEXT -log "There are still running jobs on %s!" $q
     $INFOTEXT "To make sure, that no date will be lost, the uninstall\n" \
               "of this executionhost stops now!"
     $INFOTEXT "Please, check the running jobs and run uninstall again!"
     $INFOTEXT -log "To make sure, that no date will be lost, the uninstall\n" \
               "of this executionhost stops now!"
     $INFOTEXT -log "Please, check the running jobs and run uninstall again!"
     break

   done
}

RemoveQueues()
{
   exechost=$1

   for q in `qstat -f | grep $exechost | cut -d" " -f1`; do

     $INFOTEXT "Deleting queue %s!" $q
     $INFOTEXT -log "Deleting queue %s!" $q
     
     for hgrp in `qconf -shgrpl`; do
       `qconf -shgrp $hgrp >> hgrp.tmp`
       if [ `qconf -shgrp $hgrp | grep $exechost | wc -w` = 2 ]; then
          `cat hgrp.tmp | sed s/$exechost/NONE/ >> hgrp2.tmp`
       else
          `cat hgrp.tmp | sed s/$exechost// >> hgrp2.tmp`
       fi
       `qconf -Mhgrp hgrp2.tmp`
       `rm hgrp.tmp`
       `rm hgrp2.tmp`
     done

   done

}


RemoveExecd()
{
   exechost=$1

   $INFOTEXT "Removing exec host %s now!" $exechost
   $INFOTEXT -log "Removing exec host %s now!" $exechost

   qconf -dh $exechost
   qconf -ds $exechost
   qconf -ke $exechost
   qconf -de $exechost
 

}

RemoveSpoolDir()
{
   exechost=$1

   $INFOTEXT "Checking local spooldir configuration!\n"

   SPOOL_DIR=`qconf -sconf $exechost | grep execd_spool_dir | awk '{ print $2 }'`
   
   if [ -z "$SPOOL_DIR" ]; then
      $INFOTEXT "Checking global spooldir configuration!\n"
      SPOOL_DIR=`qconf -sconf | grep execd_spool_dir | awk '{ print $2 }'`
   fi
  
   HOST_DIR=`echo $exechost | tr "[A-Z]" "[a-z]"` 
   
   $INFOTEXT "Removing spool directory [%s]" $SPOOL_DIR/$HOST_DIR
   ExecuteAsAdmin `rm -R $SPOOL_DIR/$HOST_DIR`

   if [ `ls -la $SPOOL_DIR | wc -l` -lt 4 ]; then
      ExecuteAsAdmin `rm -R $SPOOL_DIR`
   fi

}
