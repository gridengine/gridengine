#!/bin/sh -x

new_hostname=$1
home=$2

/bin/touch $home
/bin/chmod 777 $home
echo hostname -l h=$new_hostname >> $home
echo uname -l h=$new_hostname >> $home
