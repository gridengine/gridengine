#!/bin/sh

PATH=/bin:/usr/bin

ARCH=`$SGE_ROOT/util/arch`
HOST=`$SGE_ROOT/utilbin/$ARCH/gethostname -name`

end=false
while [ $end = false ]; do

   # ----------------------------------------
   # wait for an input
   #
   read input
   result=$?
   if [ $result != 0 ]; then
      end=true
      break
   fi

   if [ "$input" = "quit" ]; then
      end=true
      break
   fi

   echo "begin"
   echo "$HOST:test1:6"
   echo "end"
done

