#!/bin/sh

#
# look at the user/admin provided qtask files to find
# submission defaults for the job type
# 
QtaskOpts()
{
   cmd_name=$1
   adm_qtask_file=$SGE_ROOT/default/common/qtask
   usr_qtask_file=$HOME/.qtask

   if [ -r $adm_qtask_file ]; then
      qtask_line=`grep "^\!$cmd_name" $adm_qtask_file`
      if [ $? -eq 0 ]; then
         if [ ! "x$qtask_line" = "x" ]; then
            echo "$qtask_line" | cut -f2- -d" "
            exit
         fi
      fi
   fi 

   if [ -r $usr_qtask_file ]; then
      qtask_line=`grep "^$cmd_name" $usr_qtask_file`
      if [ $? -eq 0 ]; then
         if [ ! "x$qtask_line" = "x" ]; then
            echo "$qtask_line" | cut -f2- -d" "
            exit
         fi
      fi
   fi
}

# retrieve binary architecture for starting qsub command
#
arch=`$SGE_ROOT/util/arch`
if [ $? -ne 0 ]; then
   echo "qbsub: failed to retrieve architecture" >&2
   exit 1
fi

# to figure out which argument is the script name 
# we parse all qsub options 
#
qsub_opts=""
while [ "$1" != "" ]; do
   case "$1" in
   -clear|-cwd|-hard|-h|-notify|-soft|-V)
         qsub_opts="$qsub_opts $1"
         shift
         ;;
   -a|-ac|-A|-c|-ckpt|-C|-dc|-e|-hold_jid|-j|-l|-m|-M|-masterq|-now|-N|-o|-p|-q|-r|-sc|-S|-v|-W)
      if [ $# -lt 2 ]; then
         echo "qsub: syntax error with option $1" >&2
         exit 1
      fi
      qsub_opts="$qsub_opts $1 $2"
      shift 2
      ;;
   -pe)
      if [ $# -lt 3 ]; then
         echo "qsub: syntax error with option $1" >&2
         exit 1
      fi
      qsub_opts="$qsub_opts $1 $2 $3"
      shift 3
      ;;
   *)
      break;
      ;;
   esac
done


if [ $# -gt 0 ]; then
   cmd=$1
   cmd_name=`basename $1`
   qtask_opts="`QtaskOpts $cmd_name`"
   shift
   if [ $# -gt 0 ]; then
      dashdash=--
   else
     dashdash=""
   fi
   echo "echo $cmd | $SGE_ROOT/bin/$arch/qsub -N $cmd $qtask_opts $qsub_opts $dashdash $*"
   echo $cmd | $SGE_ROOT/bin/$arch/qsub -N $cmd $qtask_opts $qsub_opts $dashdash $*
else
   qtask_opts=""
   $SGE_ROOT/bin/$arch/qsub $qsub_opts
fi
