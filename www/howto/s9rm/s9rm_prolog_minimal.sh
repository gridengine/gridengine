#!/bin/sh
# (c) 2002 Sun Microsystems, Inc. Use is subject to license terms.  
# Copyright © 2002 Sun Microsystems, Inc.  All rights reserved.
#****** s9rm_prolog_minimal.sh *********************************************
#
#  NAME
#     s9rm_prolog_minimal.sh -- prolog to associate a job with a Solaris 9
#     Resource Manager task.
#
#  SYNOPSIS
#     s9rm_prolog_minimal.sh 
#
#  FUNCTION
#     This script can be used as prolog in sge_queue(5), to create
#     a new taskid for the job.
# 
#  NOTES
#     The /usr/bin/newtask command is not available before Solaris 8.
#
#***************************************************************************
if [ ! -x /usr/bin/newtask ]
then
	echo "Warning: /usr/bin/newtask is not available, skipping Solaris 9 Resource Manager setup." 
	exit 0
fi
####
####  The line below creates a new task for this job.
####
/usr/bin/newtask -c `/bin/ps -o ppid= -p $$`
####
exit 0
#### ### ### End of the prolog ### ### ###
