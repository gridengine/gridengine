#!/bin/sh
# (c) 2002 Sun Microsystems, Inc. Use is subject to license terms.  
# Copyright © 2002 Sun Microsystems, Inc.  All rights reserved.
#****** s9rm_starter_method.sh *********************************************
#
#  NAME
#     s9rm_starter_method.sh -- prolog to associate a job with a Solaris 9
#     Resource Manager task.
#
#  SYNOPSIS
#     s9rm_starter_method.sh 
#
#  FUNCTION
#     This script can be used as starter_method in sge_queue(5) to create
#     a new taskid for the job.
# 
#  NOTES
#     The /usr/bin/newtask command is not available before Solaris 8.
#
#***************************************************************************
if [ -x /usr/bin/newtask ]
then
   SM_DEFAULT_PROJECT="`/bin/projects -d ${USER}`"
   exec /usr/bin/newtask -p $SM_DEFAULT_PROJECT $SGE_STARTER_SHELL_PATH $*
else
   echo "Warning: /usr/bin/newtask is not available, skipping Solaris 9 Resource Manager setup." 
   exec $SGE_STARTER_SHELL_PATH $*
fi
#### ### ### End of the starter_method ### ### ###
