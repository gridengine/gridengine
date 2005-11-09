#!/bin/sh

new_hostname=$1
home=$2

touch $home
chmod 777 $home
echo hostname -l h=$new_hostname >> $home
echo !uname -l h=$new_hostname >> $home
