#!/bin/sh

# This script does not work at midnight, because it assumes that the hours
# don't roll over.
duration=$1

if [ "$duration" = "" ]; then
   duration=5
fi

time=`date '+%H:%M:%S'`
bhour=`echo $time | awk -F: '{print $1}'`
bminute=`echo $time | awk -F: '{print $2}'`
bsecond=`echo $time | awk -F: '{print $3}'`
btotal=`expr $bhour \* 3600 + $bminute \* 60 + $bsecond`
eduration=`expr $btotal + $duration`
aduration=$btotal

while [ $aduration -lt $eduration ]
do
   duration=`expr $eduration - $aduration`
   echo Sleeping $duration seconds
   /bin/sleep $duration

   time=`date '+%H:%M:%S'`
   ehour=`echo $time | awk -F: '{print $1}'`
   eminute=`echo $time | awk -F: '{print $2}'`
   esecond=`echo $time | awk -F: '{print $3}'`
   etotal=`expr $ehour \* 3600 + $eminute \* 60 + $esecond`
   aduration=$etotal
done
