#!/bin/sh

#
# renew_cred - renew Codine/GRD Kerberos credentials
#
# This script should be executed as a root process on the qmaster machine
# and on any execution hosts supporting Kerberos and running the Kerberos
# security option. The script renews the Kerberos credentials for
# Codine/GRD jobs.
#

getfileuser() {
   set -- `ls -l $1`
   echo $3
}

getfilegroup() {
   set -- `ls -l $1`
   echo $4
}

sleep_time=900                  # time to sleep between renewing TGTs
kinit='/usr/krb5/bin/kinit'     # path to kinit binary

cd /tmp
while /bin/true
do

   x=`find . -name 'krb5cc_sge_*' -o \
             -name 'krb5cc_qmaster_*'`

   for a in $x
   do
      echo Renewing $a
      if cp -p $a $a.tmp
      then
         user=`getfileuser $a`
         group=`getfilegroup $a`
         KRB5CCNAME=FILE:/tmp/$a.tmp; export KRB5CCNAME
         if $kinit -R 
         then
            chown $user $a.tmp
            chgrp $group $a.tmp
            mv $a.tmp $a
         else
            rm $a.tmp
         fi
      fi
   done

   sleep $sleep_time

done


