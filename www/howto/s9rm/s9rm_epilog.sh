#!/bin/sh
# Copyright © 2002 Sun Microsystems, Inc.  All rights reserved.
# (c) 2002 Sun Microsystems, Inc. Use is subject to license terms.  
#****** util/resources/s9rm_epilog.sh ***************************************
#
#  NAME
#     s9rm_epilog.sh -- epilog to generate accounting reports at 
#			the end of a job 
#
#  SYNOPSIS
#     s9rm_epilog.sh 
#
#  FUNCTION
#     This script can be used as epilog in sge_queue(5); It runs a 
#     a program to generate a summary report from the job's exacct records.
# 
#  NOTES
#     The /usr/bin/newtask command is not available before Solaris 8.
#     Please set the EPILOG_SUMMARY variable to the path of report generator 
#     generator program before using this script. 
#
#***************************************************************************
####
#### Name the program that generates a summary of the 
#### job's exacct records
####
EPILOG_SUMMARY=<<path to report generator>>/proclist
####
####
EPILOG_MYTASK="`/bin/ps -o taskid= -p $$`"
#### You might want to call newtask again here, to finish the previous task,
#### otherwise the exacct task record will not be available at this point
if [ -x $EPILOG_SUMMARY ]
then 
	$EPILOG_SUMMARY
fi
####
