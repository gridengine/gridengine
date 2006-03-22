#!/bin/sh

PROGRAMS="qacct qalter qconf qdel qhold qhost qlogin qmake qmod qmon qping qresub qrls qrsh qselect qsh qstat qsub qtcsh"
PROGRAMS="$PROGRAMS sge_coshepherd sge_execd sgepasswd sge_qmaster sge_schedd sge_shadowd sge_shepherd"
PROGRAMS="$PROGRAMS adminrun checkprog checkuser filestat fstype gethostbyaddr gethostbyname gethostname getservbyname infotext loadcheck now openssl qrsh_starter rlogin rsh rshd sge_share_mon spooldefaults spooledit spoolinit testsuidroot uidgid"

CLEAN=$1
shift
BASEDIR=$1
shift

# clear directory, if requested
if [ $CLEAN -eq 1 ]; then
   rm -rf $BASEDIR
fi

# create log base directory
if [ ! -d $BASEDIR ]; then
   mkdir -p $BASEDIR
   chmod 777 $BASEDIR
fi

# create logfiles for all users
for user in $*; do
   USERDIR=$BASEDIR/$user
   if [ ! -d $USERDIR ]; then
      mkdir -p $USERDIR
      chgrp staff $USERDIR
      chmod g+s $USERDIR
   fi
   chown $user $USERDIR
   chmod 777 $USERDIR
   for prog in $PROGRAMS; do
      logfile=$USERDIR/tca.$prog.log
      touch $logfile
      chown $user $logfile
      chgrp staff $logfile
      chmod 777 $logfile
   done
done
