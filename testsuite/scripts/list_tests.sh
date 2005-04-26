#!/bin/sh

base=checktree
versions="53 60"

find_check()
{
   dir=$1

   ret=""

   # check can be a global check for all versions
   if [ -f $dir/check.exp ]; then
      ret="all"
   else
      # or a check for special version(s)
      for version in $versions; do
         if [ -f $dir/check.$version.exp ]; then
            ret="$ret $version"
         fi
      done
   fi

   # no check found?
   if [ "x$ret" = "x" ]; then
      ret="none"
   fi

   echo $ret
}


cd $base

dirs=`find . -type directory | grep -v CVS`
for dir in $dirs; do
   if [ $dir = "." ]; then
      continue
   fi

   name=`echo $dir | cut -d / -f 2-`
   check=`find_check $name`
   if [ $check = "none" ]; then
      continue
   fi
   
   echo "$name $check"
done
