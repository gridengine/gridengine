#!/bin/sh
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
#  License at http://www.gridengine.sunsource.net/license.html
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
# Terminate an MPI job when its parent batch job is terminated
#
# Save CRE job info, which will be used by stopsunmpi.sh
#
#-----------------------------------
# GetPList
GetPList()
{
#
# Process "mpps -p" output and identify all the processes
# under a given CRE Job ID (stored at cre_jid)
# Put the output in "rsh" command string.
#
iline=1
cat $1 | while read line; do
    if [ $iline -eq 1 ]
    then
#        skip the first label for JID, the second label for Rank, 
#        or the last blank line if applicable
#         echo skip $iline -th line
       iline=`expr $iline + 1`
    elif [ $iline -eq 3 ] 
    then
#        skip the first label for JID, the second label for Rank, 
#        or the last blank line if applicable
#         echo skip $iline -th line
       iline=`expr $iline + 1`
    elif [ $iline -eq 2 ]
    then
#        Pick up JID, NPROC 
#         echo pick up $iline -th line for jid and nproc
       jid=`echo $line|cut -f1 -d" "`
       nproc=`echo $line|cut -f2 -d" "`
       mylines=`expr $nproc + 3`
       iline=`expr $iline + 1`
    else 
#        Pick up RANK, PID, NODE if JID matches with mine.
       if [ $jid = $cre_jid ]
       then
          if [ $iline -le `expr $nproc + 3` ]
          then
#               echo pick up $iline -th line for rank and others
	     rank=`echo $line|cut -f1 -d" "`
             pid=`echo $line|cut -f2 -d" "`
             node=`echo $line|nawk '{print $4}'`
#              output pid into file
#              we need to make it more robust
             echo rsh -n $node ptree $pid  
#	       echo `rsh -n $node ptree $pid | grep -v spmd | nawk '{print $1}'` 
          else
#               echo Done picking $cre_jid, reset $cre_jid
             cre_jid=-1
          fi 
       fi
       iline=`expr $iline + 1`
       if [ $iline -eq `expr $nproc + 5` ]
       then
#            set to the beginning of the next set
#            echo skip $iline -th line
	  iline=1
       fi
   fi
done
}

#----------------
# GetCreJid
#
GetCreJid()
{
  mprun_pid=`cat $TMPDIR/mprun_pid`
  adb=/usr/bin/adb
#  if [ "`isainfo -n`" = "sparcv9" ]; then
#  isainfo is not available on solaris 2.6
#
  if [ "`/bin/ls -l /opt/SUNWhpc/bin | grep HPC4.0`" != "" ]; then
#
#    HPC 4.0 release
#    A single mprun (32-bit binary) is provided
#
      mprun_prog_name=/opt/SUNWhpc/bin/mprun
  else
#
#    HPC 3.1 release
#    Two mprun binaries are provided
#
    if [ "`isalist | grep sparcv9`" != "" ]; then
      mprun_prog_name=/opt/SUNWhpc/bin/sparcv9/mprun
    else
      mprun_prog_name=/opt/SUNWhpc/bin/sparcv7/mprun
    fi
  fi
#
# we need to use adb and have a commands file
# we could also keep the file in same dir as this exec
# but not much of a perf loss to keep creating it in $TMPDIR
#
  adb_comms=$TMPDIR/adb-comms.$$
#
# It turns out that 32-bit and 64-bit mprun core 
# needs different adb commands to get CRE job id
#
  if [ "`/bin/ls -l /opt/SUNWhpc/bin | grep HPC4.0`" != "" ]; then
#
#    HPC 4.0 release
#    A single mprun (32-bit binary) is provided
#
       /usr/bin/echo "*task+4/D" > $adb_comms
  else
#
#    HPC 3.1 release
#    Two mprun binaries are provided
#
    if [ "`isalist | grep sparcv9`" != "" ]; then
       /usr/bin/echo "*task+8/D" > $adb_comms
    else
       /usr/bin/echo "*task+4/D" > $adb_comms
    fi
  fi
#
# get a core of the process and use adb to get job/task id
# would be nice to reference the variable directly
# through /proc interface
#
  corefile=$TMPDIR/mpcr
  ret=`/usr/bin/gcore -o $corefile $mprun_pid 2>&1`
  corefile=$corefile.$mprun_pid
  outputfile=$TMPDIR/adbres.$$
  $adb $mprun_prog_name $corefile < $adb_comms > $outputfile 2>&1

  cre_jid=`/usr/bin/grep tty_s_orig $outputfile | nawk '{print $2}'` 
  if [  "$cre_jid" -eq "" ]; then
    cre_jid=`/usr/bin/tail -2 $outputfile | nawk '{print $2}'| head -n 1`
  fi
#
# Clean up temporary files
#
  /bin/rm $adb_comms $corefile $outputfile
}

#
# Get CRE Job ID
#
GetCreJid
#
# List all my MPI processes 
#
mympps=$TMPDIR/mpps.p.out
/opt/SUNWhpc/bin/mpps -p > $mympps
#
# Create rsh command to get all the process trees 
# related to a given CRE Job ID
# This file will be used by stopsunmpi.sh script 
# in case there're any zombie processes
#
myptree=$TMPDIR/ptree.list
GetPList $mympps > $myptree
/bin/rm  $mympps
#
# Use mpkill to kill the MPI job
#
/opt/SUNWhpc/bin/mpkill -KILL $cre_jid

# signal success to caller
exit 0

