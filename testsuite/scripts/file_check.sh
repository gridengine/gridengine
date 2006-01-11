#!/bin/sh
#
# this script is only called from open_remote_spawn_process in tcl_files/remote_procedures.tcl
# do not change the content without having a look at open_remote_spawn_process
# we want to be sure that the script file is completely transfered to the remote host
#
FILE=$1

if [ -x $FILE ]; then
  if [ -s $FILE ]; then
     tail -1 $FILE|grep _END_OF_FILE_ > /dev/null
     if [ $? = 0 ]; then
        echo "file exists"
        exit 0
     fi
  fi
fi

echo "file not found"
exit 1
