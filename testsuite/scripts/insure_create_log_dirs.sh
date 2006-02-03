#!/bin/sh

BASEDIR=/tmp/insure
PROGRAMS="qacct qalter qconf qdel qhold qhost qlogin qmake qmod qmon qping qresub qrls qrsh qselect qsh qstat qsub qtcsh"
PROGRAMS="$PROGRAMS sge_coshepherd sge_execd sgepasswd sge_qmaster sge_schedd sge_shadowd sge_shepherd"
PROGRAMS="$PROGRAMS adminrun checkprog checkuser filestat fstype gethostbyaddr gethostbyname gethostname getservbyname infotext loadcheck now openssl qrsh_starter rlogin rsh rshd sge_share_mon spooldefaults spooledit spoolinit testsuidroot uidgid"

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
   fi
   chown $user $USERDIR
   chmod 777 $USERDIR
   for prog in $PROGRAMS; do
      logfile=$USERDIR/tca.$prog.log
      touch $logfile
      chown $user $logfile
      chmod 777 $logfile
   done
done
