#!/bin/sh
#
#
#___INFO__MARK_BEGIN__
##########################################################################
#
#  The Contents of this file are made available subject to the terms of
#  the Sun Industry Standards Source License Version 1.2
#
#  Sun Microsystems Inc., March, 2001
#
#
#  Sun Industry Standards Source License Version 1.2
#  =================================================
#  The contents of this file are subject to the Sun Industry Standards
#  Source License Version 1.2 (the "License"); You may not use this file
#  except in compliance with the License. You may obtain a copy of the
#  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
#
#  Software provided under this License is provided on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#  See the License for the specific provisions governing your rights and
#  obligations concerning the Software.
#
#  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#  Copyright: 2001 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

ErrUsage()
{
   Translate "Usage:"
   echo 
   echo "$transout $cmdname [-h {u|o|s},..] job_task_list|'all'|-u user_list|-uall"
   echo "       job_task_list         job_tasks [,job_tasks, ...]"
   echo "       job_tasks             job_id['.'task_id_range]"
   echo "       task_id_range         task_id['-'task_id[':'step]]"
   echo "       user_list             user['-'user, ...]"
   echo
   exit 1
}


Translate()
{
   if [ "$translation" = "1" ]; then
      transout=`$TRANSLATE "$1"`
      if [ "$transout" = "" ]; then
         transout="$1"
      fi
   else
      transout="$1"
   fi
}

cmdname=`basename $0`

# ensure we can execute qalter binary 
if [ "$SGE_ROOT" = "" ]; then
   Translate ": Please set the environment variable SGE_ROOT."
   echo "$cmdname $transout" 
   exit 1 
fi

Translate ": cannot execute"

if [ ! -x $SGE_ROOT/util/arch ]; then
   echo "$cmdname $transout $SGE_ROOT/util/arch"
   exit 1
fi
QALTER=$SGE_ROOT/bin/`$SGE_ROOT/util/arch`/qalter
if [ ! -x $QALTER ]; then
   echo "$cmdname $transout $QALTER"
   exit 1
fi


# setup internationalization
if [ "$TEXTDOMAINDIR" = "" ] ; then
  TEXTDOMAINDIR="$SGE_ROOT/locale"
  export TEXTDOMAINDIR
fi
GETTEXT=$SGE_ROOT/utilbin/`$SGE_ROOT/util/arch`/gettext
TRANSLATE="$GETTEXT -n --domain=gridengine -s "
translation=0
#if [ ! -f $GETTEXT ] ; then
#  echo "INFO: gettext binary not found turning localization off"
#  translation=0
#fi


# parse command line
state=start
user_list=""
all_users=""
hold_types=u   
jobid_list=""
while [ "$1" != "" ]; do
   case "$1" in
   -h)
      if [ $state != "start" ]; then
         ErrUsage
      fi
      shift
      hold_types=$1   
      shift
      state=got_hold_list
      ;;
   -u)
      shift
      user_list=$1
      shift
      ;;
   -uall)
      all_users="1"
      shift
      ;;
   [0-9,]*)
      jobid_list="$jobid_list $1"
      shift
      ;;  
   all)
      jobid_list="$jobid_list $1"
      shift
      ;;
   *)
      ErrUsage
      ;;
   esac
done

# invalid parameter - do not start 
if [ "$jobid_list" = "" -a "$user_list" = "" -a "$all_users" = "" ]; then
   Translate ": list of usernames or jids expected"
   echo "$cmdname $transout"
   ErrUsage
fi

if [ "$user_list" != "" -a "$all_users" != "" ]; then
   Translate ": selection of usernames and -uall switch are not allowed together"
   echo "$cmdname $transout"
   ErrUsage
fi

if [ \( "$user_list" != "" -o "$all_users" != "" \) -a "$jobid_list" != "" ]; then
   Translate " : selection of usernames and jids not allowed together "
   echo "$cmdname $transout"
   ErrUsage
fi

# ensure valid hold types are given
valid=`expr $hold_types : '[uso]*$'`
if [ $valid = "0" ]; then
   Translate ": invalid list of hold types"
   "echo $cmdname $transout"
   ErrUsage
fi

# map qrls hold_types into qalter hold_types
if [ "$cmdname" = "qrls" ]; then
   hold_types="`echo $hold_types|tr 'uos' 'UOS'`"
fi


# build switches for qalter
hold_param=""
user_list_param=""
jobid_list_param=""
all_users_param=""

if [ "$hold_types" != "" ]; then
   hold_param="-h $hold_types"
fi
if [ "$user_list" != "" ]; then
   user_list_param="-u $user_list"
fi
if [ "$all_users" = "1" ]; then
   all_users_param="-uall"
fi
if [ "$jobid_list" != "" ]; then
   jobid_list_param="$jobid_list"
fi

$QALTER $hold_param $user_list_param $all_users_param $jobid_list_param
