#!/bin/sh
# (c) 2002 Sun Microsystems, Inc. Use is subject to license terms.  
# Copyright © 2002 Sun Microsystems, Inc.  All rights reserved.
#****** util/resources/s9rm_prolog.sh ***************************************
#
#  NAME
#     s9rm_prolog.sh -- Prolog to set up Solaris 9 Resource Manager 
#	resource control and accounting.
#
#  SYNOPSIS
#     s9rm_prolog.sh
#
#  FUNCTION
#     This script can be used as prolog in sge_queue(5). It creates
#     a new taskid for the job, sets the user's default project as projid,
#     sets resource controls, and binds the job to a resource pool.
#     If some of this functionality is not desired, comment out the 
#     respective commands.
# 
#  NOTES
#     The /usr/bin/newtask command is not available before Solaris 8.
#
#***************************************************************************
if [ ! -x /usr/bin/newtask ]
then
	echo "Warning: /usr/bin/newtask is not available, skipping Solaris 9 resource Manager setup." 
	exit 0
fi
####  Get the shepherd's process id
SHEP_PID="`/bin/ptree $$ | awk 'BEGIN {getline; getline; print $1}'`"
####
##################################################################
####
####  Setting the jobs' task and project ids
####
##################################################################
####
####  The lines below get the user's default project name and id
####  You might want a fancier mapping of jobs/queues to projects
####
PROLOG_DEFAULT_PROJECT="`/bin/projects -d ${USER}`"
PROLOG_PROJECT_ID="`grep $PROLOG_DEFAULT_PROJECT /etc/project| /usr/bin/awk -F: '{print $2}'`"
####
####  The line below creates a new task for this job and assigns it to the
####  user's default project. 
####
/usr/bin/newtask -p $PROLOG_DEFAULT_PROJECT -c $SHEP_PID
PROLOG_JOB_TASKID="`/bin/ps -o taskid= -p $SHEP_PID`"
####
##################################################################
####
####  Binding the job to a resource pool
####
##################################################################
####
PROLOG_POOL=single
PROLOG_MYTASK="`/bin/ps -o taskid= -p $$`"
/usr/sbin/poolbind -p $PROLOG_POOL -i taskid $PROLOG_JOB_TASKID
##################################################################
####
####  Setting resource controls
####
##################################################################
####
/usr/bin/prctl -n task.max-lwps -v 9 -e signal=9 -i task $PROLOG_JOB_TASKID
####
exit 0
####
#### ### ### End of the prolog ### ### ###
