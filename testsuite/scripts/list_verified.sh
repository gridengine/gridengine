#!/bin/sh

# get filenames through stdin, e.g. from list_tests.sh

mypath=`dirname $0`

$mypath/list_tests.sh | awk '{print $1}' | while read check; do
   if [ -f checktree/$check/check.exp ]; then
      grep VERIFIED checktree/$check/check.exp >/dev/null 2>&1
      if [ $? -eq 0 ]; then
         echo $check
      fi
   else
      if [ -f checktree/$check/check.60.exp ]; then
         echo $check
      fi
   fi
done
