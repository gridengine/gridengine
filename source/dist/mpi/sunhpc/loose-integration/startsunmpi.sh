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
# preparation of the mpi machine file
#
# usage: startsunmpi.sh [options] <pe_hostfile> <mprun_path>
#
#        options are: 
#                     -catch_hostname 
#                      force use of hostname wrapper in $TMPDIR when starting mprun   
#                     -catch_rsh
#                      force use of rsh wrapper in $TMPDIR when starting mprun   
#                     -unique
#                      generate a machinefile where each hostname appears only once
#                      This is needed to setup a multithreaded mpi application
#

PeHostfile2MachineFile()
{
if [ "$allocation_rule" = "\$round_robin" ] 
then
	    cat $1 | nawk '
                { line[NR] = $0
                }
	        END { ncpus=0
                      for (i = 1; i < NR+1; i++) {
                          n = split(line[i], temp, " ")
                          ncpus += temp[2]
                      }

                      for (i = 0; i < ncpus; i++) {
			    irem = i
			    irem %= NR
			    ipos = irem + 1
                             n = split(line[ipos], temp, " ")
  			     m = split(temp[1], lessdn, ".")
                             print lessdn[1] " 1"
                    }
               }' 

else
   cat $1 | while read line; do
      # echo $line
      host=`echo $line|cut -f1 -d" "|cut -f1 -d"."`
      nslots=`echo $line|cut -f2 -d" "`
      i=1
      while [ $i -le $nslots ]; do
         # add here code to map regular hostnames into ATM hostnames
         # Add a unit number after hostname for mprun -Mf format 
         echo $host 1
         i=`expr $i + 1`
      done
   done
fi

}


#
# startup of MPI conforming with the Grid Engine
# Parallel Environment interface
#
# on success the job will find a machine-file in $TMPDIR/machines
# 

# useful to control parameters passed to us  
# echo $*

# parse options
catch_rsh=0
catch_hostname=0
unique=0
while [ "$1" != "" ]; do
   case "$1" in
      -catch_rsh)
         catch_rsh=1
         ;;
      -catch_hostname)
         catch_hostname=1
         ;;
      -unique)
         unique=1
         ;;
      *)
         break;
         ;;
   esac
   shift
done

me=`basename $0`

# test number of args
if [ $# -ne 2 ]; then
   echo "$me: got wrong number of arguments" >&2
   exit 1
fi

# get arguments
pe_hostfile=$1
MPIR_HOME=$2
export MPIR_HOME

# ensure job will be able to exec mprun
if [ ! -x $MPIR_HOME/mprun ]; then
   echo "$me: can't execute $MPIR_HOME/mprun" >&2
   exit 1
fi

# ensure pe_hostfile is readable
if [ ! -r $pe_hostfile ]; then
   echo "$me: can't read $pe_hostfile" >&2
   exit 1
fi

# create machine-file
# remove column with number of slots per queue
# mpi does not support them in this form
machines="$TMPDIR/machines"

# trace machines file
# cat $pe_hostfile

mype=$TMPDIR/mype
#if [ `isainfo -b` = 64 ]; then
# isainfo is not available on Solaris 2.6.
#
if [ "`isalist | grep sparcv9`" != "" ]; then
         ARCH=solaris64
else
         ARCH=solaris
fi
$SGE_ROOT/bin/$ARCH/qconf -sp $PE > $mype
allocation_rule=`grep allocation $mype | nawk '{print $2}'`
# echo allocation_rule = $allocation_rule

if [ $unique = 1 ]; then
   PeHostfile2MachineFile $pe_hostfile | uniq >> $machines
else
   PeHostfile2MachineFile $pe_hostfile >> $machines
fi

#
# Make script wrapper for 'rsh' available in jobs tmp dir
#
if [ $catch_rsh = 1 ]; then
   rsh_wrapper=$SGE_ROOT/mpi/rsh
   if [ ! -x $rsh_wrapper ]; then
      echo "$me: can't execute $rsh_wrapper" >&2
      echo "     maybe it resides at a file system not available at this machine" >&2
      exit 1
   fi

   rshcmd=rsh
   case "$ARC" in
      hp|hp10|hp11|hp11-64) rshcmd=remsh ;;
      *) ;;
   esac
   # note: This could also be done using rcp, ftp or s.th.
   #       else. We use a symbolic link since it is the
   #       cheapest in case of a shared filesystem
   #
   ln -s $rsh_wrapper $TMPDIR/$rshcmd
fi

#
# Make script wrapper for 'hostname' available in jobs tmp dir
#
if [ $catch_hostname = 1 ]; then
   hostname_wrapper=$SGE_ROOT/mpi/hostname
   if [ ! -x $hostname_wrapper ]; then
      echo "$me: can't execute $hostname_wrapper" >&2
      echo "     maybe it resides at a file system not available at this machine" >&2
      exit 1
   fi

   # note: This could also be done using rcp, ftp or s.th.
   #       else. We use a symbolic link since it is the
   #       cheapest in case of a shared filesystem
   #
   ln -s $hostname_wrapper $TMPDIR/hostname
fi

# signal success to caller
exit 0
